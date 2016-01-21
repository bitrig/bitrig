package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"regexp"
	"strings"
)

// libcaudit is a tool that generates a csv file that contains the following
// information:
//	symbol name (same as provided)
//	man page (does this symbol have a man page)
//	proto (PROTO_* definition if it exists)
//	def (DEF_* definition if it exists)
//	alias (__strong or __weak alias type if it exists)
//	symbols (does the symbol exist in Symbols.list)
//	nm (nm output of symbol name)
//
// libcaudit must be provided a symbol file using the -in parameter.  This file
// must have exactly one symbol per line.
// This file can be generated in a number of ways.  For example:
// grep -r "\.Nm" /usr/src/lib/libc/locale/ | awk '{print $2}' | sort -u > /tmp/Nm
//
// libcaudit must also be provided the full path to the compiled libc.so file
// in order to run nm against.
//
// Example of running this tool:
// go run libcaudit.go -in /tmp/Nm -libc /usr/lib/libc.so.83.0 -v
//
// This tool is not performant.  Ease of writing won over making it run fast.

var nmResult string

func nmPrep(libc string) error {
	out, err := exec.Command("nm", libc).Output()
	if err != nil {
		return err
	}

	nmResult = string(out)
	return nil
}

func nm(l string, fo *os.File) {
	var output string

	defer func() {
		fmt.Fprintf(fo, "%v", output)
	}()

	re := regexp.MustCompile(" \\w \\S*" + l + "(\\.c\n|\n)")
	s := re.FindAllString(nmResult, -1)
	if len(s) == 0 {
		output = "invalid"
		return
	}

	for _, v := range s {
		// strip newlines
		if strings.HasSuffix(v, "\n") {
			v = v[:len(v)-1]
		}
		fmt.Fprintf(fo, v)
	}
}

func symbols(l string, fo *os.File) {
	var output string

	defer func() {
		fmt.Fprintf(fo, "%v", output)
	}()

	_, err := exec.Command("grep", "-rIw", l, "/usr/src/lib/libc/Symbols.list").Output()
	if err != nil {
		output = "no"
		return
	}
	output = "yes"
}

func alias(l string, fo *os.File) {
	var output string

	defer func() {
		fmt.Fprintf(fo, "%v", output)
	}()

	out, err := exec.Command("grep", "-rIw", l, "/usr/src/lib/libc").Output()
	if err != nil {
		output = "no"
		return
	}

	// find _alias
	o := string(out)
	i := strings.Index(o, "_alias")
	if i == -1 {
		output = "no"
		return
	}

	// walk backwards to find alias type
	var start int
	for start = i; i > -1; start-- {
		if o[start] == ':' {
			o = o[start+1:]
			break
		}
	}
	if start < 0 {
		output = "invalid"
		return
	}

	// clip at \n
	i = strings.Index(o, "\n")
	if i == -1 {
		output = "invalid"
		return
	}
	o = o[:i]

	output = o
}

func grep(l string, fo *os.File, search string) {
	var output string

	defer func() {
		fmt.Fprintf(fo, "%v", output)
	}()

	out, err := exec.Command("grep", "-rIw", l, "/usr/src/lib/libc").Output()
	if err != nil {
		output = "no"
		return
	}

	// find start of "search"
	o := string(out)
	i := strings.Index(o, search)
	if i == -1 {
		output = "no"
		return
	}
	o = o[i:]

	// clip at \n
	i = strings.Index(o, "\n")
	if i == -1 {
		output = "invalid"
		return
	}
	o = o[:i]

	output = o
}

func def(l string, fo *os.File) {
	grep(l, fo, "DEF_")
}

func proto(l string, fo *os.File) {
	grep(l, fo, "PROTO_")
}

func manPage(l string, fo *os.File) {
	var output string

	defer func() {
		fmt.Fprintf(fo, "%v", output)
	}()

	_, err := exec.Command("man", l).Output()
	if err != nil {
		output = "no"
		return
	}
	output = "yes"
}

func main() {
	in := flag.String("in", "", "function name list")
	out := flag.String("out", "result.csv", "results file")
	libc := flag.String("libc", "", "runtime libc e.g. /usr/lib/libc.so.83.0")
	verbose := flag.Bool("v", false, "print progress")
	flag.Parse()

	if *in == "" {
		fmt.Fprintf(os.Stderr, "must provide -in\n")
		os.Exit(1)
	}

	if *out == "" {
		fmt.Fprintf(os.Stderr, "must provide -out\n")
		os.Exit(1)
	}

	if *libc == "" {
		fmt.Fprintf(os.Stderr, "must provide -libc\n")
		os.Exit(1)
	}

	// names
	fi, err := os.Open(*in)
	if err != nil {
		fmt.Fprintf(os.Stderr, "os.Open: %v\n", err)
		os.Exit(1)
	}
	defer fi.Close()
	r := bufio.NewReader(fi)

	// make sure we have a valid libc
	err = nmPrep(*libc)
	if err != nil {
		fmt.Fprintf(os.Stderr, "nmPrep: %v\n", err)
		os.Exit(1)
	}

	// results file
	fo, err := os.Create(*out)
	if err != nil {
		fmt.Fprintf(os.Stderr, "os.Create: %v\n", err)
		os.Exit(1)
	}
	defer fo.Close()

	fmt.Fprintf(fo, "# name, man page, proto, def, alias, symbols, nm\n")

	for {
		l, err := r.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				break
			}
			fmt.Fprintf(os.Stderr, "r.ReadString: %v\n", err)
			os.Exit(1)
		}

		// kill newline
		if strings.HasSuffix(l, "\n") {
			l = l[:len(l)-1]
		}

		// skip garbage
		if len(l) == 0 {
			continue
		}

		fmt.Fprintf(fo, "%v", l)
		fmt.Fprintf(fo, ",")

		manPage(l, fo)
		fmt.Fprintf(fo, ",")

		proto(l, fo)
		fmt.Fprintf(fo, ",")

		def(l, fo)
		fmt.Fprintf(fo, ",")

		alias(l, fo)
		fmt.Fprintf(fo, ",")

		symbols(l, fo)
		fmt.Fprintf(fo, ",")

		nm(l, fo)

		// done
		fmt.Fprintf(fo, "\n")

		if *verbose {
			fmt.Printf("%v\n", l)
		}
	}
}
