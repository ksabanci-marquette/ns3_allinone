# LENA-NB (Narrowband)
This repository includes LENA-NB, an NB-IoT extension to ns-3. Currently, the following features are implemented:

- RRC Connection Resume Procedure (3GPP Rel. 13)
- Cellular IoT Optimization (3GPP Rel. 13)
- Early Data Transmission (3GPP Rel. 15)

- Cross-Subframe Scheduling
- Adaptive Modulation and Coding
- NB-IoT Energy State Machine

Currently, no NB-IoT specific error model is used. Instead, MATLAB was used to provide adequate configurations depending on a given SNR [1,2]. 
A lookup table is derived from MATLAB NB-IoT BLER simulations to find a decent UL and DL configuration corresponding to a target BLER.
In future work, an NB-IoT specific error model will be integrated in LENA-NB.



# How to use LENA-NB 

LENA-NB is based on the ns-3 release 3.32. We recommend using LENA-NB with ns-3 release 3.32, since other releases may have introduced changes in the source code 
which may interfere with our provided code. 
After downloading and building ns-3 the original lte folder, found in ns3/bake/source/ns-3.32/src, is replaced with the lte folder provided in this respository. 
Note that currently LENA-NB replaces the ns-3 LTE implementation. In future releases a coexistence will be introduced. 

# First Steps 

We have included an example script, called lena-nb-5G-scenario.cc, which can be used for testing the NB-IoT implementation in ns-3. This script expects the 
following parameters:

```
--simTime       # Time to be simulated, in milliseconds
--randomSeed    # Seed for Random Number Generator
--numUeAppA     # Number of UEs for the first application / use cases
--numUeAppB     # Number of UEs for the second application / use cases
--numUeAppC     # Number of UEs for the third application / use cases
--ciot          # Flag whether Cellular IoT Optimization should be used
--edt           # Flag whether Early Data Transmission should be used
```

With these parameters NB-IoT UEs are uniformly distributed on a disc with a diameter of ```cellsize=2500m```. Each UE transmits once a day a user payload 
of ```packetsize_app_a=49 Bytes```. Tn this case the payload for App A, B and C are equal, but can be changed in the script.

## Note regarding actual Simulation Time
Note that the actual simulation time is 3*simTime. If simTime is set to 5 minutes, 15 minutes of simulation time are simulated, which is important for high-scaled 
scenarios. The first 5 minutes produce no significant results since devices at the beginning are scheduled in an empty cell and experience very good transmission 
conditions. After 5 minutes, new devices will find ongoing transmissions of previous devices, which enables a more realistic situation and produces significant 
results. Since devices that have started transmissions within the intermediate 5 minutes of the simulation may not complete their transmissions in this intermediate 
time slot, additional 5 minutes are simulated with more new transmissions to keep the channels busy and let the intermediate devices complete their transmissions.

# Automated Simulations  

For an automated execution of multiple simulations with different seeds and configurations, we provice a python script, called runner_example.py. This script 
generates the required lena-nb-5G-scenario.cc calls including all parameters. All simulations are queued using a class called taskQueue, which executes the simulations 
one-by-one and automatically restarts a simulation if an error occured. The parameter num_workers is used for parallel simulations. If the simulations are used by 
servers with e.g. 50 cores, the up to 50 workers can be set for up to 50 parallel simulations. Note that multiple parallel simulations consume much memory. 



# References

[1] MathWorks. 2021. NB-IoT NPDSCH Block Error Rate Simulation. Retrieved December 11, 2021 from https://www.mathworks.com/help/lte/ug/nb-iot-npdsch-block-error-rate-simulation.html
[2] MathWorks. 2021. NB-IoT NPUSCH Block Error Rate Simulation. Retrieved December 11, 2021 from https://www.mathworks.com/help/lte/ug/nb-iot-npusch-block-error-rate-simulation.html
