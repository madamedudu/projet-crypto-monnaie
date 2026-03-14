# Compilateur et options de compilation
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Dossiers du projet
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Nom de l'exécutable final
EXEC = bitthune
NOM_ETU = Rendu_BitThune_Phase1_Chirine_Lassaoui

# Liste des fichiers sources de la phase 1
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/blockchain.c \
          $(SRC_DIR)/block.c \
          $(SRC_DIR)/transaction.c \
          $(SRC_DIR)/utils.c \
          $(SRC_DIR)/sha256.c \
          $(SRC_DIR)/sha256_utils.c

# Liste des fichiers objets correspondants
OBJECTS = $(OBJ_DIR)/main.o \
          $(OBJ_DIR)/blockchain.o \
          $(OBJ_DIR)/block.o \
          $(OBJ_DIR)/transaction.o \
          $(OBJ_DIR)/utils.o \
          $(OBJ_DIR)/sha256.o \
          $(OBJ_DIR)/sha256_utils.o
# Règle par défaut
all: $(EXEC)

# Règle pour créer l'exécutable final en liant tous les .o
$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
	@echo "Compilation terminée avec succès ! Lancez ./$(EXEC)"

# Règle pour compiler chaque fichier .c en .o
# Crée le dossier obj/ s'il n'existe pas
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour nettoyer les fichiers compilés (utile en cas de bug)
clean:
	rm -rf $(OBJ_DIR) $(EXEC)
	@echo "Fichiers de compilation nettoyés."

# Règle pour générer l'archive zip pour le rendu
deliver: clean # le clean permet d'eviter de rendre un fichier compilé 
	mkdir -p $(NOM_ETU)NOM_EETU
	cp -r src $(NOM_ETU)
	cp -r include $(NOM_ETU)
	cp Makefile $(NOM_ETU)
	cp README.txt $(NOM_ETU)
	zip -r $(NOM_ETU).zip $(NOM_ETU)
	rm -rf $(NOM_ETU)
	@echo " "
	@echo "=================================================="
	@echo "Vous devez maintenant déposer l'archive $(NOM_ETU).zip sur Moodle."
	@echo "=================================================="