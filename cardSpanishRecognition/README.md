###################################################
          README: SPANISH DECK RECOGNITION
###################################################

This projects consists in the recognition of a complete
spanish deck of cards. A simple algorithm is used
for the sake of simplicity.

Created on: March 2016
Author: ""
Contact: Github profile


# Algorithm
* A simple pre-processing in the original image is taken: Some Gaussian noise is added and a simple binarization.
* Every card in the image is processed separately from the rest. We seek to crop every one of them to have the same orientation (upwards or downwards) and the same size. The corners are also removed for they provide additional information.
* Adaptive binarization of those individual cards. Comparison with the database.

# Further work
* Robust treatment of light. Pretty good at the moment, but still some sun reflects may disturb the classification.
