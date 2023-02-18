# Max Clique

## Build

[`max_clique.cpp`](./max_clique.cpp) is the main file and contains the implementation. To build a CLI application you need to run the command below:

```bash
g++ -std=c++17 -O3 max_clique.cpp
```

Please, pay attention that app requires **C++ 17**. You should use one of the [compilers that support](https://en.cppreference.com/w/cpp/compiler_support/17) the standard.

## Report

The output of one program run can be found in **below** or in [`clique.csv`](./clique.csv).

```text
            Instance    Clique      Time, sec
      brock200_1.clq        19           0.53
      brock200_2.clq         9          0.066
      brock200_3.clq        12          0.143
      brock200_4.clq        15           0.24
      brock400_1.clq        23          1.444
      brock400_2.clq        24          1.185
      brock400_3.clq        23          1.191
      brock400_4.clq        24          1.114
          C125.9.clq        33          1.619
  gen200_p0.9_44.clq        43          2.865
  gen200_p0.9_55.clq        54           3.25
      hamming8-4.clq        16          0.167
   johnson16-2-4.clq         8          0.066
    johnson8-2-4.clq         4          0.001
         keller4.clq        11           0.07
        MANN_a27.clq       125        105.616
         MANN_a9.clq        16          0.115
     p_hat1000-1.clq         8          0.247
     p_hat1000-2.clq        45          6.371
     p_hat1500-1.clq        10          0.766
      p_hat300-3.clq        34          1.415
      p_hat500-3.clq        48          5.084
         san1000.clq         9          5.012
     sanr200_0.9.clq        40          2.277
     sanr400_0.7.clq        19            0.6
```

P.S.: Thank you for reading!
