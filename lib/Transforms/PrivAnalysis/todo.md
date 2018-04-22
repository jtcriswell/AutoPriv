# A list of things that have better to be done but not in a hurry.

#### remove accidentally added priveleges to the binary
If the administrator of the system accidentally added unneeded capabilities
to the executable binary, then the PrivAnalysis pass needes to remove them.
This can be done after the LocalAnalysis pass is finished when we know
what privileges are in the permissible set and what privileges are really 
used by *priv_raise* calls.
