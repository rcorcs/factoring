
run()
{
   while [ $count -ge 0 ]
   do
      date
      echo "Count: " $count
      ./factoring -r $digit >> $out
      (( count-- ))
   done
}


count=9
digit=35
out="output2.log"
while [ $digit -le 50 ]
do
   count=9
   echo "Digits: " $digit
   run
   digit=$[$digit+5]
done

count=9
digit=5
out="output3.log"
while [ $digit -le 50 ]
do
   count=9
   echo "Digits: " $digit
   run
   digit=$[$digit+5]
done

