struct TCPHeader{
	int seq_num;
	int flow_id;
	int src_id;
	double sender_timestamp;
	double receiver_timestamp;
};