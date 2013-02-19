
all:
	lcm-gen -j orc_message_t.lcm
	lcm-gen -j orc_status_t.lcm
	javac *.java
	java -cp .:$(CLASSPATH) ComputeHashes
