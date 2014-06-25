#################################################
#
# Makefile progetto lso 2010
# (fram 1)(fram 2)(fram 3)
#################################################

# ***** DA COMPLETARE ******  con i file da consegnare
# primo frammento
FILE_DA_CONSEGNARE1=./genList.c ./genHash.c 

# secondo frammento
FILE_DA_CONSEGNARE2=./logpro 

# terzo frammento
FILE_DA_CONSEGNARE3= \
../doc/relazione.pdf \
blockingList.c \
blockingList.h \
blockingList_itr.c \
blockingList_itr.h \
blockingList_private.h \
comsock.c \
comsock.h \
errorUtil.h \
genHash.c \
genHash.h \
genList.c \
genListElem.c \
genListElem.h \
genList.h \
listElem.h \
logpro \
Makefile \
msgcli.c \
msgserv.c \
msgserv.h \
msgservWorkerReceiver.c \
msgservWorkerSender.c \
msgservWriter.c \
myTest_blockingList.c \
myTest_comsock.c \
myTest_taskResult.c \
myTest_threadPool.c \
taskResult.c \
taskResult.h \
taskResult_private.h \
threadPool.c \
threadPool.h \
threadPool_private.h \
threadPool_task.c \
threadPool_task.h \
threadPool_task_private.h \
threadPool_thread.c \
threadPool_thread.h \
threadPool_thread_private.h \

MYTEST_DIR = ../testMy

########################################################################
# COMPILATORE
########################################################################

#defines per le stampe di debug
DBGALL  = -Ddbg_msgserv -Ddbg_msgcli -Ddbg_threadPool -D_notest
DBGALL_ = -Ddbg_msgserv -Ddbg_msgcli -Ddbg_threadPool
DBGSERV = -Ddbg_msgserv
DBGCLI  = -Ddbg_msgcli
DBGMSG  = -Ddbg_msgserv -Ddbg_msgcli
DBGTDPL = -Ddbg_threadPool
DNOTEST = -D_notest
DBG = 

# Compiler flags
CFLAGS = -Wall -pedantic -g $(DBG)
# aggiungere -lgcov o -fprofile-arcs al linking

# Compilatore
CC = gcc

LIB_CFLAGS = -L$(LIB_DIR)

########################################################################
# SORGENTI
########################################################################

# Directory dei files sorgenti
SRC_DIR = ./src

########################################################################
# LIBRERIE
########################################################################

# directory installazione librerie 
LIB_DIR = ../lib

# Nome libreria
MSG_LIBNAME = libmsg.a

# Le mie librerie
BCOLLECTION_LIBNAME = libblockingCollection.a

TDPL_LIBNAME = libthreadPool.a

# tutte le librerie
LIBS = $(MSG_LIBNAME) $(BCOLLECTION_LIBNAME) $(TDPL_LIBNAME)


########################################################################
# HEADERS FILE
########################################################################

# directory installzaione librerie
HEADERS_DIR = ../include

# Tutti gli headers pubblici
HEADERS = threadPool_task.h threadPool.h \
 taskResult.h listElem.h genList.h genListElem.h \
 genHash.h errorUtil.h comsock.h blockingList_itr.h blockingList.h


########################################################################
# BINARI
########################################################################

# directory installazzione eseguibili
BIN_DIR = ../bin

# eseguibili
BINS = msgserv msgcli

OBJS_DIR = ../binTmp

OBJS_ALL = 

# Lista degli object files (** DA COMPLETARE ***)
OBJS = genList.o genHash.o

# nomi eseguibili test primo frammento
exe1 = msg_test1
exe2 = msg_test2




# primo parametro di tutti i miei test, decide se stampare tutte le
# informazioni dei test ('p') oppure no ('t')
myTests_print_arg = p


# phony targets
.PHONY: clean lib test11 test12 docu consegna1
.PHONY: test21 consegna2
.PHONY: test31 test32 test33 consegna3
.PHONY: install uninstall libThreadPool libBCollection
.PHONY:	exe_myTest_genList exe_myTest_genHash exe_myTest_blockingList \
  exe_myTest_taskResult exe_myTest_threadPool exe_myTest_comsock

install: 
# HEADERS
	-mkdir $(HEADERS_DIR)/
	-cp -f $(HEADERS) $(HEADERS_DIR)/

# LIBRERIE
	-mkdir $(LIB_DIR)/
	make libBCollection
	make libThreadPool
	make lib

# BINARI
	-mkdir $(BIN_DIR)/
	make msgserv
	-cp -f msgserv $(BIN_DIR)/
	make msgcli
	-cp -f msgcli $(BIN_DIR)/

uninstall:
# HEADERS
	-cd $(HEADERS_DIR)/ && rm -f $(HEADERS)
	-rmdir $(HEADERS_DIR)

# LIBRERIE
	-cd $(LIB_DIR)/ && rm -f $(LIBS)
	-rmdir $(LIB_DIR)/

# BINARI
	make clean
	-rm -f $(BIN_DIR)/msgserv
	-rm -f $(BIN_DIR)/msgcli
	-rmdir $(BIN_DIR)/



########################################################################
# REGOLE PER LA CREAZIONE DELLE LIBRERIE
########################################################################

# creazione libreria msg
lib:  $(OBJS)
	-mkdir $(LIB_DIR)/
	-rm  -f $(LIB_DIR)/$(MSG_LIBNAME)
	ar -r $(MSG_LIBNAME) $(OBJS)
	cp $(MSG_LIBNAME) $(LIB_DIR)/

libBCollection:
	make $(LIB_DIR)/$(BCOLLECTION_LIBNAME)

libThreadPool: 
	make $(LIB_DIR)/$(TDPL_LIBNAME)


$(LIB_DIR)/$(MSG_LIBNAME): 
	make lib


########################################################################
# REGOLE PER LA LIBRERIE MSG e BINARI
########################################################################

genList.o: genList.c genList.h

genHash.o: genHash.c genHash.h

# eseguibile di test 1 (primo frammento)
$(exe1): genList.o test-genList.o
	$(CC) -o $@ $^ 

# eseguibile di test 2 (primo frammento)
$(exe2): $(OBJS) test-genHash.o
	$(CC) -o $@ $^ 

# dipendenze oggetto main di test 11
test-genList.o: test-genList.c genList.h
	$(CC) $(CFLAGS) -c $<

# dipendenze oggetto main di test 12
test-genHash.o: test-genHash.c genHash.h genList.h
	$(CC) $(CFLAGS) -c $<

# make rule per gli altri .o del primo frammento (***DA COMPLETARE***)

# make rule per i .o del terzo frammento (***DA COMPLETARE***)

msgserv.o: msgserv.c errorUtil.h comsock.h genHash.h genList.h \
  genListElem.h threadPool.h threadPool_task.h taskResult.h \
  blockingList.h blockingList_itr.h listElem.h \

msgserv: msgserv.o msgservWriter.o msgservWorkerReceiver.o \
 msgservWorkerSender.o comsock.o \
  $(LIB_DIR)/$(MSG_LIBNAME) $(LIB_DIR)/$(TDPL_LIBNAME) \
  $(LIB_DIR)/$(BCOLLECTION_LIBNAME)
	$(CC) -o $@ $^ $(LIB_CFLAGS) -lmsg -lpthread -lblockingCollection -lthreadPool

msgcli: msgcli.o comsock.o $(LIB_DIR)/$(MSG_LIBNAME)
	$(CC) -o $@ $^ $(LIB_CFLAGS) -lmsg -lpthread

comsock.o: comsock.c comsock.h


########################################################################
# REGOLE PER I MIEI TEST 
########################################################################

#### LIBRERIA MSG ####

#myTest_genList.o: 

#myTest_genHash.o:

#myTest_genList:

#myTest_genHash:

#exe_myTest_genList:
#	make clean
#	make myTest_genList
#	./myTest_genList

#exe_myTest_genHash:
#	make clean
#	make myTest_genHash
#	./myTest_genHash

#### COMSOCK ####

myTest_comsock.o: myTest_comsock.c errorUtil.h comsock.h

myTest_comsock: myTest_comsock.o comsock.o
	$(CC)   -o $@ $^   -lpthread

exe_myTest_comsock:
	make clean
	make myTest_comsock 
	./myTest_comsock 

#### LIBRERIA BLOCKINGCOLLECTION ####

myTest_blockingList.o: myTest_blockingList.c blockingList.h \
  blockingList_itr.h listElem.h errorUtil.h

myTest_blockingList: myTest_blockingList.o blockingList.o \
  blockingList_itr.o
	$(CC) -o $@ $^ -lpthread

# lanciare a mano i test con thread, con ./myTest_blockingList p 5 6
# richiedono qualche minuto (da modificare) TODO
exe_myTest_blockingList:
	make clean
	make myTest_blockingList
	./myTest_blockingList $(myTests_print_arg) 0 4

####  LIBRERIA THREAD POOL ####

myTest_taskResult.o: myTest_taskResult.c taskResult.h errorUtil.h

myTest_threadPool.o: myTest_threadPool.c threadPool.h threadPool_task.h \
 taskResult.h errorUtil.h


myTest_taskResult: myTest_taskResult.o taskResult.o
	$(CC)   -o $@ $^   -l pthread

myTest_threadPool: myTest_threadPool.o threadPool.o threadPool_thread.o \
  threadPool_task.o taskResult.o blockingList.o blockingList_itr.o
	$(CC)   -o $@ $^   -l pthread


exe_myTest_taskResult: 
	make clean
	make myTest_taskResult
	./myTest_taskResult $(myTests_print_arg)

exe_myTest_threadPool:
	make clean
	make myTest_threadPool 
	./myTest_threadPool $(myTests_print_arg)

########################################################################
# TUTTI I MIEI TEST
########################################################################

myTests_:  myTest_blockingList myTest_taskResult myTest_threadPool \
  myTest_comsock
#	./myTest_genList
#	./myTest_genHash
	./myTest_blockingList
	./myTest_taskResult
	./myTest_threadPool
	./myTest_comsock

myTests:
#	make exe_myTest_genList
#	make exe_myTest_genHash
	make exe_myTest_blockingList
	make exe_myTest_taskResult
	make exe_myTest_threadPool
	make exe_myTest_comsock


########################################################################
# REGOLE PER LA LIBRERIA BLOCKINGCOLLECTION
########################################################################

BCOLLECTION_OBJS = blockingList.o blockingList_itr.o

blockingList.o: blockingList.c blockingList.h blockingList_private.h \
  listElem.h

blockingList_itr.o: blockingList_itr.c blockingList_itr.h blockingList.h \
  listElem.h blockingList_private.h

$(LIB_DIR)/$(BCOLLECTION_LIBNAME): $(BCOLLECTION_OBJS)
	-rm  -f ./$(BCOLLECTION_LIBNAME)
	-rm  -f $(LIB_DIR)/$(BCOLLECTION_LIBNAME)
	ar -r $(BCOLLECTION_LIBNAME) $^
	cp $(BCOLLECTION_LIBNAME) $(LIB_DIR)/

########################################################################
# REGOLE PER LA LIBRERIA THREAD POOL
########################################################################

TDPL_OBJS = threadPool.o taskResult.o threadPool_task.o \
 threadPool_thread.o 

taskResult.o: taskResult.c taskResult.h taskResult_private.h

threadPool_task.o: threadPool_task.c threadPool_task.h taskResult.h \
  threadPool_task_private.h

threadPool_thread.o: threadPool_thread.c threadPool_thread.h \
  threadPool_task.h taskResult.h

threadPool.o: threadPool.c threadPool.h threadPool_task.h taskResult.h \
  threadPool_private.h blockingList.h threadPool_thread.h \
  threadPool_task_private.h blockingList_itr.h listElem.h

$(LIB_DIR)/$(TDPL_LIBNAME): $(TDPL_OBJS)
	-rm  -f $(TDPL_LIBNAME)
	-rm  -f $(LIB_DIR)/$(TDPL_LIBNAME)
	ar -r $(TDPL_LIBNAME) $^
	cp $(TDPL_LIBNAME) $(LIB_DIR)/



########### NON MODIFICARE DA QUA IN POI ################
# genera la documentazione con doxygen
docu: ../doc/Doxyfile
	make -C ../doc

#ripulisce
clean:
	-rm -f *.o *~

clear:
	make clean
	-rm -f msgserv msgcli lib*.a myTest_blockingList \
		myTest_taskResult myTest_threadPool myTest_comsock \
		.err* .out* outcli* out1* out2* out3* *.check

# primo test primo frammento
test11: 
	make clean
	make $(exe1)
	echo MALLOC_TRACE e\' $(MALLOC_TRACE)
	@echo MALLOC_TRACE deve essere settata a \"./.mtrace\"
	-rm -f ./.mtrace
	./$(exe1)
	mtrace ./$(exe1) ./.mtrace
	@echo -e "\a\n\t\t *** Test 1-1 superato! ***\n"

# secondo test primo frammento
test12: 
	make clean
	make $(exe2)
	echo MALLOC_TRACE e\' $(MALLOC_TRACE)
	@echo MALLOC_TRACE deve essere settata a \"./.mtrace\"
	-rm -f ./.mtrace
	./$(exe2)
	mtrace ./$(exe2) ./.mtrace
	@echo -e "\a\n\t\t *** Test 1-2 superato! ***\n"

# test secondo frammento
test21: 
	cp DATA/logtest.* .            
	chmod 644 ./logtest.*       
	./logpro ./logtest.dat > ./outtest    
	diff ./outtest ./logtest.check  
	@echo -e "\a\n\t\t *** Test 2-1 superato! ***\n"

# primo test terzo frammento 
test31: msgserv msgcli lib
	-killall -w msgserv  
	-killall -w msgcli   
	cp DATA/userlist .   
	cp DATA/userlist2 .  
	cp DATA/out31.check . 
	chmod 644 userlist userlist2 out31.check
	-rm -fr tmp/   
	-mkdir tmp 
	./testparse 2>.err 
	diff ./out31 ./out31.check 
	@echo -e "\a\n\t\t *** Test finale 3-1 superato! ***\n" 

# secondo test terzo frammento 
test32: msgserv msgcli lib  
	-killall -w msgserv  
	-killall -w msgcli   
	cp DATA/userlist .   
	cp DATA/outcli?.check . 
	chmod 644 ./userlist 
	chmod 644 ./outcli?.check
	-rm -fr tmp/          
	-mkdir tmp 
	./testfunz 
	diff ./outcli1 ./outcli1.check 
	diff ./outcli2 ./outcli2.check 
	diff ./outcli3 ./outcli3.check 
	@echo -e "\a\n\t\t *** Test finale 3-2 superato! ***\n" 

# terzo test terzo frammento 
test33: msgserv msgcli lib   
	-killall -w msgserv  
	-killall -w msgcli   
	cp DATA/userlist .   
	cp DATA/logsort.check .   
	chmod 644 userlist  
	chmod 644 logsort.check
	-rm -fr tmp/           
	-mkdir tmp 
	./testpar  
	@echo -e "\a\n\t\t *** Test finale 3-3 superato! ***\n" 

# target di consegna primo frammento
SUBJECT1="lso10: consegna primo frammento"
consegna1: 
	make test11
	make test12
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f1.tar ./gruppo.txt ./Makefile $(FILE_DA_CONSEGNARE1)
	mpack -s $(SUBJECT1) ./$(USER)-f1.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Frammento 1 consegnato  ***\n"

# target di consegna secondo frammento
SUBJECT2="lso10: consegna secondo frammento"
consegna2: 
	make test21
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f2.tar ./gruppo.txt $(FILE_DA_CONSEGNARE2)
	mpack -s $(SUBJECT2) ./$(USER)-f2.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Frammento 2 consegnato  ***\n"

# target di consegna terzo frammento
SUBJECT3="lso10: consegna terzo frammento"
consegna3: 
	make test11
	make test12
	make test21
	make test31
	make test32
	make test33
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f3.tar ./gruppo.txt $(FILE_DA_CONSEGNARE3)
	mpack -s $(SUBJECT3) ./$(USER)-f3.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Progetto finale consegnato  ***\n"

creaTarConsegna:
	tar -cvf $(USER)-f3-prova.tar ./gruppo.txt $(FILE_DA_CONSEGNARE3)

tests:
	make test11
	make test12
	make test21
	make test31
	make test32
	make test33

createtar:
	tar cjf msg_$(paramdate)__$(paramname)__.tar.bz2 *.c *.h Makefile 

#end
