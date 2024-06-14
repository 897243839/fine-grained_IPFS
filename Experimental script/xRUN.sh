#！ /bin/bash
echo $1
filename=$1
#网络带宽30Mbps 60Mb 100Mb 500Mb
ratelist=(
30
60
100
500
)
ip=192.168.0.33
name_ip=ipfsm12@${ip}
sudo tc qdisc del dev eno1 root
for rate  in ${ratelist[@]}
    do
        echo ${name_ip}
        echo ${rate}
        sshpass -p f123456789 ssh ${name_ip} "sudo tc qdisc del dev eno1 root"
        sshpass -p f123456789 ssh ${name_ip} "sudo tc qdisc add dev eno1 root netem rate ${rate}mbit delay 10ms"
        sleep 2
        ping $ip -c 5 >>ping_${filename}_${rate}Mb.txt
        echo "TCP">>ping_${filename}_${rate}Mb.txt
        iperf3 -c $ip -p 12345 -R -i 1 -t 5 >>ping_${filename}_${rate}Mb.txt
        sleep 2
        echo "UDP">>ping_${filename}_${rate}Mb.txt
        iperf3 -c $ip -p 12345 -R -i 1 -t 5 -u -b 600M >>ping_${filename}_${rate}Mb.txt
        sleep 2
        ./silesia-yfget.sh ${filename}-${rate}Mb
        ./TMD-yfget.sh ${filename}-${rate}Mb
	./SVHN-yfget.sh ${filename}-${rate}Mb
	./EMNIST-yfget.sh ${filename}-${rate}Mb
    done
sshpass -p f123456789 ssh ${name_ip} "sudo tc qdisc del dev eno1 root"








