#！ /bin/bash
echo $1
sar_filename=$1
function timediff() {
    
    #time format:date +"%s.%N", such as 1502758855.907197692
    start_time=$1
    end_time=$2
    
    start_s=${start_time%.*}
    start_nanos=${start_time#*.}
    end_s=${end_time%.*}
    end_nanos=${end_time#*.}
    
    #end_nanos > start_nanos?
    #Another way, the time part may start with 0, which means
    #it will be regarded as oct format, use "10#" to ensure
    #calculateing with decimal
    if [ "$end_nanos" -lt "$start_nanos" ];then
        end_s=$(( 10#$end_s - 1 ))
        end_nanos=$(( 10#$end_nanos + 10**9 ))
    fi
    
    
    time=$(( 10#$end_s - 10#$start_s )).`printf "%03d\n" $(( (10#$end_nanos - 10#$start_nanos)/10**6 ))`
    
    echo $time
}

function clear_cache(){
    
    echo "--------clear cache starting--------"
    sync;
    sudo bash -c "echo 1 > /proc/sys/vm/drop_caches"
    sudo bash -c "echo 2 > /proc/sys/vm/drop_caches"
    sudo bash -c "echo 3 > /proc/sys/vm/drop_caches"
    echo "--------clear cache successfully--------"
}

namelist=(  
QmXhB3D6yA6vBRDddrnCfq6w6evS1a6DppaGRnHqR5BYV7
 )


getlist=(  
EMNIST
)
j=0
for n in {1..5}
do
     clear_cache
     startz=$(date +"%s.%N")
     for a  in ${namelist[@]}
        do
            ipfs get  $a 
        done
      endz=$(date +"%s.%N")
      tz=$(timediff $startz $endz)
        echo "第$n次,EMNIST, $tz", s" ,$startz, $endz, " >>EMNIST-${sar_filename}.csv
        sleep 120
        ipfs pin ls --type recursive | cut -d' ' -f1 | xargs -n1 ipfs pin rm
        ipfs repo gc
        rm -r Qm*
	rm Qm*
	echo "第$n次,EMNIST, $tz"
	sleep 30
    j=$j+1
done






