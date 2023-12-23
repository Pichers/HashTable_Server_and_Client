Grupo: 62
Alunos:
    António Almeida - nº 58235
    Carlos Van-Dùnem - nº 58202
    Pedro Cardoso - nº 58212


O projeto está organizado de acordo com a seguinte estrutura:

    Diretorio "grupo62-projeto3": contem todas as subpastas do projeto, o ficheiro makefile e o ficheiro README
        Subdiretorio "bin": contem os executáveis após a compilação através do make
        Subdiretorio "dependencies": contem os ficheiros das dependencias .d
        Subdiretorio "include": contem os ficheiros .h
        Subdiretorio "obj": contem os ficheiros .o compilados pelo makefile, e os pre-compilados.
        Subdiretorio "src": que contém of ficheiros .c

Decisões de implementação:
    Foi escolhido criar um header file privado, mutex-private.h, responsável por partilhar as variáveis necessárias para a sincronização.
    Foi utilizada a biblioteca pthreads do UNIX.
    O atendimento de clientes segue o modelo thread-per-client.
    Foi implementada uma função no servidor para que o servidor desligue, libertando toda a memória, aquando do comando ctrl-c.
    Também no cliente foi implementada uma função com o mesmo propósito.

