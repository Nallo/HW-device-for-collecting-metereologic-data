# Questo Makefile presuppone la sua presenza ed esecuzione dalla cartella SRC
# N.B. eventuali @ davanti ad un comando, indicano di non generare messaggi sul terminale che esegue il comando


# Nome dell'eseguibile generato a fine compilazione
PROGRAM   = Raspberry_Gruppo_1

# Directory di hidapi
HIDAPIDIR =   ./hidapi-0.7.0/linux
# Directory di JsonBox
JSONBOXDIR  =   ./JsonBox/src

# Directory dove generare l'eseguibile finale
BINDIR    =   ../BIN
# Path delle librerie esterne su cui appoggiarsi
LIBDIR    =   ../LIB
# Path dove generare i file oggetto
OBJDIR    =   ../OBJ/Raspberry_Gruppo_1

# Lista di oggetti da linkare
# wildcard da una lista dei file .cpp rilevati
# patsubst sostituisce le occorrenze di .cpp con .o
# addprefix aggiunge un prefisso
OBJ = $(addprefix $(OBJDIR)/, $(patsubst %.cpp, %.o, $(wildcard *.cpp)))

# Flags per il compilatore
CXXFLAGS = -std=c++11 -Wall -pthread -lcurl
# Compilatore da usare
CXX = g++-4.7

# Include di qualche header particolare
INCLUDES = -I/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss/include

# Librerie referenziate
LDFLAGS = /usr/lib/arm-linux-gnueabihf/libpthread.so `pkg-config libusb-1.0 libudev --libs` $(LIBDIR)/libhidapi.a $(LIBDIR)/libJsonBox.a /usr/local/lib/libcurl.so -ltbb -Wl,-rpath,/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss/build/linux_armv6l_gcc_cc4.6_libc2.13_kernel3.6.11+_release -L/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss/build/linux_armv6l_gcc_cc4.6_libc2.13_kernel3.6.11+_release

# Regola all: esegue le regole segnate
all: make_hidapi make_jsonbox mkobj $(OBJDIR) mkbin $(PROGRAM)

# Compila hidapi
make_hidapi:
	@sudo make -C $(HIDAPIDIR) -f Makefile-Raspberry_Gruppo_1 all

# Compila JsonBox
make_jsonbox:
	@sudo make -C $(JSONBOXDIR) all

# Crea la cartella degli obj, senza generare warning se esiste gia'
mkobj:
	@mkdir -p $(OBJDIR)

# Crea la cartella bin, senza generare warning se esiste gia'
mkbin:
	@mkdir -p $(BINDIR)

# Per ogni file .cpp, esegue una compilazione nel path della regola (-o $@). $< indica il primo prerequisito, che e' appunto il file .cpp
$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Per ogni file .o, esegue il link con i vari pattern richiesti. (@^ indica tutti i prerequisiti) 
$(PROGRAM): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS) -o $(BINDIR)/$@

# Clean, con rimozione dei file oggetto e dell'eseguibile finale
clean:
	@rm -rf $(OBJDIR)
	@rm -f $(BINDIR)/$(PROGRAM)

# Richiama il Makefile di hidapi con invocata la regola clean
clean_hidapi:
	@sudo make -C $(HIDAPIDIR) -f Makefile-Raspberry_Gruppo_1 clean

# Richiama il Makefile di JsonBox con invocata la regola clean
clean_jsonbox:
	@sudo make -C $(JSONBOXDIR) clean

# Clean di me stesso, di g2log e di hidapi
clean_all: clean_hidapi clean_jsonbox clean

# Esegue il programma
run:
	$(BINDIR)/$(PROGRAM) 
