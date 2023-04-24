all: test_pcap pcap

test_pcap: test_pcap.c
	gcc test_pcap.c -o test_pcap -lpcap

pcap: pcap.c
	gcc pcap.c -o pcap -lpcap

clean:
	rm -f pcap test_pcap
