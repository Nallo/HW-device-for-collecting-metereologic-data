# Questo Makefile presuppone la sua presenza ed esecuzione dalla cartella hidapi-0.7.0/linux
# N.B. eventuali @ davanti ad un comando, indicano di non generare messaggi sul terminale che esegue il comando


# Nome della libreria da generare
LIB_OUT_NAME = libJsonBox.a

# Path dove generare la libreria
LIBDIR       = ../../../LIB
# Path dove generare i file oggetto
CXXOBJDIR      = ../../../OBJ/JsonBox

# Flags per il compilatore C++
CXXFLAGS = -std=c++11 -Wall
# Compilatore C da usare
CXX      = g++-4.7

# Lista di oggetti da linkare
# wildcard da una lista dei file .cpp rilevati
# patsubst sostituisce le occorrenze di .cpp con .o
# addprefix aggiunge un prefisso
OBJ = $(addprefix $(CXXOBJDIR)/, $(patsubst %.cpp, %.o, $(wildcard *.cpp)))

all: mkobjcxx mklib $(CXXOBJDIR) $(LIB_OUT_NAME)

# Crea la cartella degli obj, senza generare warning se esiste gia'
mkobjcxx:
	@mkdir -p $(CXXOBJDIR)

# Crea la cartella lib, senza generare warning se esiste gia'
mklib:
	@mkdir -p $(LIBDIR)

# Per ogni file .cpp, esegue una compilazione nel path della regola (-o $@). $< indica il primo prerequisito, che e' appunto il file .c
$(CXXOBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -I../include/ $< -o $@

# Per ogni file .o, esegue il link in una libreria statica .a. Prima di farlo, elimina l'eventuale libreria preesistente
$(LIB_OUT_NAME): $(OBJ)
	@rm -f $(LIBDIR)/$@
	ar cq $(LIBDIR)/$@ $(OBJ) -o $(LIBDIR)/$@

# Clean, con rimozione dei file oggetto e della libreria finale
clean:
	@rm -rf $(CXXOBJDIR)
	@rm -rf $(LIBDIR)/$(LIB_OUT_NAME)
