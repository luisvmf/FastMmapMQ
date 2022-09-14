all: $(OBJS)
	@echo "______________________"
	@echo "|       Options       |"
	@echo "| make node-module    |"
	@echo "| make python-module  |"
	@echo "| make c-demo         |"
	@echo "|_____________________|"
node-module:
	cd src&&./build-node.sh
python-module:
	cd src&&./build-python.sh
c-demo:
	gcc -o receive_example src/fastmmapmq.c receive_example.c
	gcc -o send_example src/fastmmapmq.c send_example.c
	gcc -o test src/fastmmapmq.c test.c
