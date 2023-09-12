ARGS=8 5 20 

all: client server supervisor
client: client.c
	@gcc client.c -o client
	@echo MAKING CLIENT
server: server.c
	@gcc server.c -lrt -pthread -o server
	@echo MAKING SERVER
supervisor: supervisor.c server
	@gcc supervisor.c -lrt -pthread -o supervisor
	@echo MAKING SUPERVISOR
clean:
	@echo REMOVING FILES
	@rm -f client server supervisor statClient.txt statSupervisor.txt
test: all
	@rm -f statClient.txt statSupervisor.txt
	@echo LAUNCHING SUPERVISOR 
	@./supervisor 8  > statSupervisor.txt & 
	@sleep 2 
	@echo LAUNCHING CLIENTS 
	@for number in 1 2 3 4 5 6 7 8 9 10; do \
		./client $(ARGS) >> statClient.txt & 	\
		./client $(ARGS) >> statClient.txt & 	\
		sleep 1 ; \
	done
	@for num in 1 2 3 4 5 6 ; do \
		sleep 10 ; \
		pkill --signal SIGINT supervisor ; \
	done
	@pkill --signal SIGINT supervisor &
	@sleep 2
	@echo LAUNCHING MISURA.SH
	@./misura.sh statClient.txt statSupervisor.txt	
	@echo TEST DONE
