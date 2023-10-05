BIN_DIR = bin
INC_DIR = include
OBJ_DIR = obj
SRC_DIR = src
DEP_DIR = dependencies
TEST_DIR = ../Projeto1_Testes
LIB_DIR = lib



CC = gcc
CFLAGS = -Wall -MMD -MP -MF $(DEP_DIR)/$*.d -I $(INC_DIR)

EXECS = $(BIN_DIR)/test_list


make: $(EXECS)


$(BIN_DIR)/test_list: $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/list.o  $(OBJ_DIR)/test_list.o
	$(CC) $^ -o $@
	


# $(BIN_DIR)/test_aluno: $(OBJ_DIR)/test_aluno.o $(OBJ_DIR)/aluno.o
# 	$(CC) $^ -o $@
	
	


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	

include $(wildcard $(DEP_DIR)/*.d)

clean:
	rm -rf $(DEP_DIR)/* $(OBJ_DIR)/* $(BIN_DIR)/*


# $(BIN_DIR)/test_aluno: $(OBJ_DIR)/test_aluno.o $(OBJ_DIR)/aluno.o
#     $(CC) $^ -o $@

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
#     $(CC) $(CFLAGS) -c $< -o $@

# include $(wildcard $(DEP_DIR)/*.d)



#     gcc Projeto1/src/data.c Projeto1/src/entry.c Projeto1/src/list.c Projeto1_Testes/test_list.c -o test_list
