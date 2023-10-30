BIN_DIR = bin
INC_DIR = include
OBJ_DIR = obj
SRC_DIR = src
DEP_DIR = dependencies



CC = gcc
CFLAGS = -Wall -MMD -MP -g -MF $(DEP_DIR)/$*.d -I $(INC_DIR)

EXECS = $(BIN_DIR)/table_client $(BIN_DIR)/table_server 


all: $(OBJ_DIR) $(DEP_DIR) $(files) $(EXECS)



$(OBJ_DIR):
	mkdir $(OBJ_DIRj)

$(DEP_DIR):
	mkdir $(DEP_DIR)

$(files):
	ifeq (,$(wildcard $(OBJ_DIR)/*))
		cp -r ./stor/* ./obj
	endif

$(BIN_DIR)/table_server: $(OBJ_DIR)/table_server.o $(OBJ_DIR)/table_skel.o $(OBJ_DIR)/network_server.o $(OBJ_DIR)/sdmessage.pb-c.o $(OBJ_DIR)/table.o $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/list.o
	$(CC) $^ -o $@ -lprotobuf-c 


$(BIN_DIR)/table_client: $(OBJ_DIR)/table_client.o $(OBJ_DIR)/network_client.o $(OBJ_DIR)/client_stub.o $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/table.o $(OBJ_DIR)/list.o $(OBJ_DIR)/network_client.o $(OBJ_DIR)/sdmessage.pb-c.o
	$(CC) $^ -o $@ -lprotobuf-c  



$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@
    

include $(wildcard $(DEP_DIR)/*.d)

clean:
	# rm -rf $(DEP_DIR)/* $(OBJ_DIR)/* $(BIN_DIR)/*
	rm -rf $(DEP_DIR)/* $(OBJ_DIR)/* $(BIN_DIR)/table_client $(BIN_DIR)/table_server