BIN_DIR = bin
INC_DIR = include
OBJ_DIR = obj
SRC_DIR = src
DEP_DIR = dependencies
LIB_DIR = lib



CC = gcc
CFLAGS = -Wall -MMD -MP -g -MF $(DEP_DIR)/$*.d -I $(INC_DIR)

EXECS = $(BIN_DIR)/test_table $(BIN_DIR)/test_list $(BIN_DIR)/test_data $(BIN_DIR)/test_entry  $(BIN_DIR)/test_serialization 


make: $(EXECS)


$(BIN_DIR)/test_list: $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/list.o  $(OBJ_DIR)/test_list.o
	$(CC) $^ -o $@

$(BIN_DIR)/test_data: $(OBJ_DIR)/data.o $(OBJ_DIR)/test_data.o
	$(CC) $^ -o $@

$(BIN_DIR)/test_entry: $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/test_entry.o
	$(CC) $^ -o $@

$(BIN_DIR)/test_table: $(OBJ_DIR)/data.o $(OBJ_DIR)/list.o $(OBJ_DIR)/table.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/test_table.o
	$(CC) $^ -o $@

$(BIN_DIR)/test_serialization: $(OBJ_DIR)/serialization.o $(OBJ_DIR)/data.o $(OBJ_DIR)/list.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/test_serialization.o
	$(CC) $^ -o $@    




$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@
    

include $(wildcard $(DEP_DIR)/*.d)

clean:
	rm -rf $(DEP_DIR)/* $(OBJ_DIR)/* $(BIN_DIR)/*