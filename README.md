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
      brock200_1.clq        19          0.014
      brock200_2.clq         9          0.009
      brock200_3.clq        12           0.01
      brock200_4.clq        15          0.011
      brock400_1.clq        22          0.052
      brock400_2.clq        22          0.051
      brock400_3.clq        22           0.05
      brock400_4.clq        22           0.05
          C125.9.clq        33          0.006
  gen200_p0.9_44.clq        36          0.014
  gen200_p0.9_55.clq        39          0.015
      hamming8-4.clq        16          0.015
   johnson16-2-4.clq         8          0.004
    johnson8-2-4.clq         4              0
         keller4.clq        11          0.007
        MANN_a27.clq       125          0.045
         MANN_a9.clq        16          0.001
     p_hat1000-1.clq         9           0.13
     p_hat1000-2.clq        45          0.245
     p_hat1500-1.clq         9          0.295
      p_hat300-3.clq        33          0.027
      p_hat500-3.clq        44          0.085
         san1000.clq        10          0.226
     sanr200_0.9.clq        39          0.016
     sanr400_0.7.clq        19          0.047
        MANN_a45.clq       342          0.391
```

P.S.: Thank you for reading!
