Grupo: 62
Alunos:
    António Almeida - nº 58235
    Carlos Van-Dùnem - nº 58202
    Pedro Cardoso - nº 58212


O projeto está organizado de acordo com a seguinte estrutura:

    Diretorio "grupo62-projeto4": contem todas as subpastas do projeto, o ficheiro makefile e o ficheiro README
        Subdiretorio "bin": contem os executáveis após a compilação através do make
        Subdiretorio "dependencies": contem os ficheiros das dependencias .d
        Subdiretorio "include": contem os ficheiros .h
        Subdiretorio "obj": contem os ficheiros .o compilados pelo makefile, e os pre-compilados.
        Subdiretorio "src": que contém of ficheiros .c

Requisitos para a utilização do Projeto4 (final):
    É necessário ter instalado o apache-zookeeper localmente.
    É necessário inicializar um servidor zookeeper antes da conexão a um servidor.

Limitações de implementação:
	Os primeiros dois servidores não podem ser inicializados em simultâneo, pelo que se deve esperar que o primeiro inicialize totalmente.
    Não é utilizado o IP externo, pelo que é apenas possível conectar servidores dentro da própria máquina.

