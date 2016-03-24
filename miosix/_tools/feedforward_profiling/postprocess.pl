#!/usr/bin/perl

my $A_b, $A_SP_Tp, $A_Tp, $B_b, $B_SP_Tp, $B_Tp, $SP_Tr, $Tr;

while(<>)
{
	if(/a->schedData\s+=\s+{priority\s+=\s+{priority\s+=\s+\d+},\s+bo\s+=\s+(\d+),\s+alfa\s+=\s+(\d+\.?\d*),\s+SP_Tp\s+=\s+(\d+),\s+Tp\s+=\s+(\d+)/) { $A_b= $2>0 ? $1/200 : 0; $A_SP_Tp=$2>0 ? $3/100 : 0; $A_Tp=$2>0 ? $4/100 : 0; }
	if(/b->schedData\s+=\s+{priority\s+=\s+{priority\s+=\s+\d+},\s+bo\s+=\s+(\d+),\s+alfa\s+=\s+(\d+\.?\d*),\s+SP_Tp\s+=\s+(\d+),\s+Tp\s+=\s+(\d+)/) { $B_b= $2>0 ? $1/200 : 0; $B_SP_Tp=$2>0 ? $3/100 : 0; $B_Tp=$2>0 ? $4/100 : 0; }
	if(/\s+SP_Tr = (\d+)/) { $SP_Tr=$1; }
	if(/\s+Tr = (\d+)/)
	{
		$Tr=$1;
		print("$A_b $A_SP_Tp $A_Tp $B_b $B_SP_Tp $B_Tp $SP_Tr $Tr\n");
	}
}
