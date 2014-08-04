run()
{
   while [ $count -ge 0 ]
   do
      date
      echo "Count: " $count
      mpiexec -n 2 mpi_factoring -r $digit >> $out
      (( count-- ))
   done
}


run_fast()
{
   while [ $count -ge 0 ]
   do
      date
      echo "Count: " $count
      mpiexec -n 2 fast_mpi_factoring -r $digit >> $out
      (( count-- ))
   done
}


count=9
digit=5
out="output.log"
while [ $digit -le 50 ]
do
   count=9
   echo "Digits: " $digit
   run
   digit=$[$digit+5]
done

count=9
digit=5
out="output.log"
while [ $digit -le 50 ]
do
   count=9
   echo "Digits: " $digit
   run
   digit=$[$digit+5]
done

