my $count=0, my $sum=0;
while(<STDIN>)
{
    next unless(/time:(\d.\d+)/);
    $sum+=$1*1000; $count++;
}
my $average=$sum/$count;
my $speed=32*1024/$average;
print "Average write time= $average ms\nAverage write speed=$speed KB/s\n";
