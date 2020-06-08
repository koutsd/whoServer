all: whoServer master worker whoClient

whoServer: whoServer.o Util.o ServerUtil.o
	g++ -std=c++11 -pthread -o whoServer whoServer.o Util.o ServerUtil.o

whoServer.o: whoServer.cpp
	g++ -std=c++11 -c whoServer.cpp

ServerUtil.o: SourceFiles/ServerUtil.cpp
	g++ -std=c++11 -c SourceFiles/ServerUtil.cpp

whoClient: whoClient.o Util.o
	g++ -std=c++11 -pthread -o whoClient whoClient.o Util.o

whoClient.o: whoClient.cpp
	g++ -std=c++11 -c whoClient.cpp

master: master.o worker Util.o
	g++ -std=c++11 -o master master.o Util.o

master.o: master.cpp
	g++ -std=c++11 -c master.cpp

worker: worker.o Util.o
	g++ -std=c++11 -o worker worker.o Hash_Table.o Max_Heap.o AVL_Tree.o PatientList.o PatientRecord.o Util.o

Util.o: SourceFiles/Util.cpp
	g++ -std=c++11 -c SourceFiles/Util.cpp

worker.o: worker.cpp Hash_Table.o
	g++ -std=c++11 -c worker.cpp

Hash_Table.o: SourceFiles/Hash_Table.cpp AVL_Tree.o Max_Heap.o
	g++ -std=c++11 -c SourceFiles/Hash_Table.cpp

Max_Heap.o: SourceFiles/Max_Heap.cpp
	g++ -std=c++11 -c SourceFiles/Max_Heap.cpp

AVL_Tree.o: SourceFiles/AVL_Tree.cpp PatientList.o
	g++ -std=c++11 -c SourceFiles/AVL_Tree.cpp

PatientList.o: SourceFiles/PatientList.cpp PatientRecord.o
	g++ -std=c++11 -c SourceFiles/PatientList.cpp

PatientRecord.o: SourceFiles/PatientRecord.cpp
	g++ -std=c++11 -c SourceFiles/PatientRecord.cpp


clean:
	rm -rf *.o worker master whoServer whoClient