 
clear;
off=fscanfMat("off.txt");
on=fscanfMat("on.txt");
off=off/100;
on=on/100;

clf; scf(0);

plot([off,on]);
title("eTr when nominal round time is 8 milliseconds (green=with feedforward)");
xlabel("rounds");
ylabel("eTr in milliseconds");
