Group: 62
Students:
    António Almeida - No. 58235
    Carlos Van-Dùnem - No. 58202
    Pedro Cardoso - No. 58212

This project implements a Hash Table Server and a Client.
The server connects to a zookeeper server, to mantain various servers connected to use chain replication.
The server and respective clients use ProtoBuffers for serialization, according to the sdmessage.proto file.

The Client receives as arguments the Zookeeper address and its port: table_client *zookeeperIP*:*zookeeperPort*
The Server receives as arguments its port, the number of lists in the hash table, and the zookeper address: table_server *port* *No. of lists* *zookeeperIP*:*zookeeperPort*

The project is organized according to the following structure:
    Directory "grupo62-projeto1": contains all project subfolders, the makefile, and the README file
        Subdirectory "bin": contains the executables after compilation using make
        Subdirectory "dependencies": contains the .d dependency files
        Subdirectory "include": contains the .h header files
        Subdirectory "obj": contains the .o object files
        Subdirectory "src": contains the .c source files

