/* n_tlim_2.t:  Tests of translation limits.   */

/* tlim.1L:  Number of parameters of macro definition: at least 127. */

#define glue127(    \
    a0, b0, c0, d0, e0, f0, g0, h0, i0, j0, k0, l0, m0, n0, o0, p0, \
    a1, b1, c1, d1, e1, f1, g1, h1, i1, j1, k1, l1, m1, n1, o1, p1, \
    a2, b2, c2, d2, e2, f2, g2, h2, i2, j2, k2, l2, m2, n2, o2, p2, \
    a3, b3, c3, d3, e3, f3, g3, h3, i3, j3, k3, l3, m3, n3, o3, p3, \
    a4, b4, c4, d4, e4, f4, g4, h4, i4, j4, k4, l4, m4, n4, o4, p4, \
    a5, b5, c5, d5, e5, f5, g5, h5, i5, j5, k5, l5, m5, n5, o5, p5, \
    a6, b6, c6, d6, e6, f6, g6, h6, i6, j6, k6, l6, m6, n6, o6, p6, \
    a7, b7, c7, d7, e7, f7, g7, h7, i7, j7, k7, l7, m7, n7, o7)     \
    a0 ## b0 ## c0 ## d0 ## e0 ## f0 ## g0 ## h0 ## \
    p0 ## p1 ## p2 ## p3 ## p4 ## p5 ## p6 ## o7

/* tlim.2L:  Number of arguments of macro call: at least 127.    */

/*  A0B0C0D0E0F0G0H0P0P1P2P3P4P5P6O7;   */
    glue127(
    A0, B0, C0, D0, E0, F0, G0, H0, I0, J0, K0, L0, M0, N0, O0, P0,
    A1, B1, C1, D1, E1, F1, G1, H1, I1, J1, K1, L1, M1, N1, O1, P1,
    A2, B2, C2, D2, E2, F2, G2, H2, I2, J2, K2, L2, M2, N2, O2, P2,
    A3, B3, C3, D3, E3, F3, G3, H3, I3, J3, K3, L3, M3, N3, O3, P3,
    A4, B4, C4, D4, E4, F4, G4, H4, I4, J4, K4, L4, M4, N4, O4, P4,
    A5, B5, C5, D5, E5, F5, G5, H5, I5, J5, K5, L5, M5, N5, O5, P5,
    A6, B6, C6, D6, E6, F6, G6, H6, I6, J6, K6, L6, M6, N6, O6, P6,
    A7, B7, C7, D7, E7, F7, G7, H7, I7, J7, K7, L7, M7, N7, O7);
