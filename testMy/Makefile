########################################################################
# Makefile miei test del progetto lso 2010
########################################################################


# le variabili DBG, SRC_DIR, LIB_DIR, HEADERS_DIR possono essere
# fornite in ingresso 

# Compiler flags
CFLAGS = -Wall -pedantic -g $(DBG)

# Compilatore
CC = gcc

LIB_CFLAGS = -L$(SRC_DIR)



SRC_DIR = ../src



MSG_LIBNAME = msg

BCOLLECTION_LIBNAME = blockingCollection

TDPL_LIBNAME = threadPool


.PHONY: clean blockingList_test_in

blockingList_test_in: 
	make clean
	make myTest_blockingList
	./myTest_blockingList $(prm1) $(prm2) $(prm3)

myTest_blockingList.o: myTest_blockingList.c $(SRC_DIR)/blockingList.h \
  $(SRC_DIR)/blockingList_itr.h $(SRC_DIR)/listElem.h $(SRC_DIR)/errorUtil.h

myTest_blockingList: myTest_blockingList.o $(SRC_DIR)/lib$(BCOLLECTION_LIBNAME).a
	$(CC)   -o $@ $<  $(LIB_CFLAGS) -lpthread -l$(BCOLLECTION_LIBNAME)

clean:
	-rm -f *.o *~


#END
