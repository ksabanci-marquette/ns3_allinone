For a detailed propagation modeling based on empirical measurements from this century, we have added the Winner+ Channel Models to the NS-3 propagation models. This code is based on the models provided in [1].

Please note that currently the implementation is limited to hexagonal layouts and an indoor distance of 3m.

When devices are placed indoors, but O2I scenarios should not be used, then a fixed additional pathloss for indoor penetration is added, given as following:
- A UE height of 0m indicates indoor placement. This will add 15.4 dB loss to the calculated pathloss, derived from [2]
- A UE height below 0.0m indicates deep indoor / basement placement. This will add 20.9 dB loss to the calculated pathloss, derived from [2]



[1] J. Meinila, et al. "D5. 3: WINNER+ final channel models." Wireless World Initiative New Radio WINNER, 2010, pp. 119-172.

[2] S. Monhof, S. Böcker, J. Tiemann, and C. Wietfeld, “Cellular Network Coverage Analysis and Optimization in Challenging Smart Grid Environments,” in 2018 IEEE International Conference on Communications, Control, and Computing Technologies for Smart Grids (SmartGridComm), 2018, pp. 1–6.
