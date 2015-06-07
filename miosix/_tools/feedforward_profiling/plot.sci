 
clear;
off=fscanfMat("off.txt");
on=fscanfMat("on.txt");
reinit=fscanfMat("reinit.txt");
off=off/100;
on=on/100;
reinit=reinit/100;

clf; scf(0);

plot([off,on,reinit]);
title("eTr with round time 8 ms (green=feedforward, red=feedforward+reinit)");
xlabel("rounds");
ylabel("eTr in milliseconds");
