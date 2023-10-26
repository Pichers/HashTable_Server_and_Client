BIN_DIR = bin
INC_DIR = include
OBJ_DIR = obj
SRC_DIR = src
DEP_DIR = dependencies
LIB_DIR = lib



CC = gcc
CFLAGS = -Wall -MMD -MP -g -MF $(DEP_DIR)/$*.d -I $(INC_DIR)

EXECS = $(BIN_DIR)/table_server $(BIN_DIR)/table_client 

make: $(EXECS)

$(BIN_DIR)/table_server: $(OBJ_DIR)/data.o 
	$(CC) $^ -o $@

$(BIN_DIR)/table_client: $(OBJ_DIR)/serialization.o 
	$(CC) $^ -o $@    




$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@
    

include $(wildcard $(DEP_DIR)/*.d)

clean:
	rm -rf $(DEP_DIR)/* $(OBJ_DIR)/* $(BIN_DIR)/*