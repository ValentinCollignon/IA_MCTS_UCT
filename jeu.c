/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 1		// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))
#define c sqrt(2)

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ? 

	char plateau[6][7];


} Etat;

// Definition du type Coup
typedef struct {
	int colonne;


} Coup;

// Copier un état 
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;
	
	int i,j;	
	for (i=0; i< 6; i++)
		for ( j=0; j<7; j++)
			etat->plateau[i][j] = src->plateau[i][j];
	

	
	return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));
	
	int i,j;	
	for (i=0; i< 6; i++)
		for ( j=0; j<7; j++)
			etat->plateau[i][j] = ' ';
	
	return etat;
}


void afficheJeu(Etat * etat) {

	int i,j;
	printf("   |");
	for ( j = 0; j < 7; j++)   //affichage des numéros de colonne
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");
	
	for(i=0; i < 6; i++) {
		printf(" %d |", 5-i);
		for ( j = 0; j < 7; j++) 
			printf(" %c |", etat->plateau[5-i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}
}


// Nouveau coup 
Coup * nouveauCoup( int i ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

	coup->colonne = i;
	
	return coup;
}

// donne la 1ére ligne libre pour une colonne donnée (return 7 si pas libre)
int caseLibre(int i,Etat * etat){
	int l = 0;
	while(l<6){ 
		if(etat ->plateau[l][i] == ' '){
			return l;
		}
		l++;
	}
	return l;
}


// Demander à l'humain quel coup jouer 
Coup * demanderCoup () {

	int i;
	printf("\n quelle colonne ? ") ;
	scanf("%d",&i); 
	
	return nouveauCoup(i);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	if ( etat->plateau[5][coup->colonne] != ' ' )
		return 0;
	else {
		int ligne = caseLibre(coup->colonne, etat);
		etat->plateau[ligne][coup->colonne] = etat->joueur ? 'O' : 'X';
		
		// à l'autre joueur de jouer
		etat->joueur = AUTRE_JOUEUR(etat->joueur); 	

		return 1;
	}	
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {
	
	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );
	
	int k = 0;

	int i;
	for(i=0; i < 7; i++) {
		if ( etat->plateau[5][i] == ' ' ) {
			coups[k] = nouveauCoup(i); 
			k++;
		}
	}
	
	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud 
typedef struct NoeudSt {
		
	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici
	
	Etat * etat; // etat du jeu
			
	struct NoeudSt * parent; 
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste
	
	// POUR MCTS:
	int nb_victoires; //w
	int nb_simus; //n
	
} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));
	
	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;			
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);		
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0; 
	}
	noeud->parent = parent; 
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;	
	

	return noeud; 	
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);
		
	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup); 

	free(noeud);
}
	

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {
	
	// tester si un joueur a gagné
	int i,j,k,n = 0;
	for ( i=0;i < 6; i++) {
		for(j=0; j < 7; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups joués
			
				// lignes
				k=0;
				while ( k < 4 && i+k < 6 && etat->plateau[i+k][j] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < 4 && j+k < 7 && etat->plateau[i][j+k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && i+k < 6 && j+k < 7 && etat->plateau[i+k][j+k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && i+k < 6 && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;		
			}
		}
	}

	// et sinon tester le match nul	
	if ( n == 6*7 ) 
		return MATCHNUL;
		
	return NON;
}



// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	Coup ** coups;
	Coup ** choix;
	Coup * meilleur_coup ;
	
	FinDePartie tmp;
	
	//coups jouable
	clock_t tic, toc;
	tic = clock();
	int temps;
	int i,m,k;

	choix = coups_possibles(etat);
	
	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	Noeud * enfant;	
	racine->etat = copieEtat(etat); 
	
	int iter = 0;
	int indiceEnfant;
	double bValue,bValue2;
	Noeud * select;
	int numJoueur;
	int indice[7];
	int p = 0;
	int coupGagnant=0;

	for(i = 0;i < 7;i++){
	  indice[i] = -1;
	}
	
	do {
	    select = racine;
	    numJoueur = etat->joueur;
	    
	    while(testFin(select->etat) == NON)//tans qu'on est pas a la fin du jeux
	    {

	      //expansion si pas d'enfant
	      if(select->nb_enfants == 0){
		  coups = coups_possibles(select->etat);
		  k = 0;
		  while(coups[k] != NULL){
		    enfant = ajouterEnfant(select,coups[k]);
		    k++;
		  }
	      }


		//Question 3
		coupGagnant=0;
		if(etat->joueur != numJoueur){
			i=0;
			while(i<select->nb_enfants && coupGagnant ==0){
				if(testFin(select->enfants[i]->etat) == ORDI_GAGNE){
					select = select->enfants[i];
					coupGagnant = 1;
				}
				i++;
			}
		}
					
	      if(coupGagnant ==0){
	      // on explore les enfants non exploré si il y en as
		      if(select->nb_enfants > select->nb_simus){
			for(i = 0;i < select->nb_enfants;i++){
			  if(select->enfants[i]->nb_simus == 0){
			    indice[p] = i;
			    p++;
			  }
			}

			select = select->enfants[ indice[rand()%p] ]; // choix aléatoire
			//reinitialisation p
			p = 0;	
		      }
		      else{
			//sinon : selection de la meilleur b-value
			indiceEnfant = 0;
			//calcul bValue
			if(etat->joueur != numJoueur)
			  bValue = (select->enfants[indiceEnfant]->nb_victoires/(double)select->enfants[indiceEnfant]->nb_simus) + (c* sqrt(log(select->enfants[indiceEnfant]->parent->nb_simus)/(double)select->enfants[indiceEnfant]->nb_simus));
			else
			  bValue = -(select->enfants[indiceEnfant]->nb_victoires/(double)select->enfants[indiceEnfant]->nb_simus) + (c* sqrt(log(select->enfants[indiceEnfant]->parent->nb_simus)/(double)select->enfants[indiceEnfant]->nb_simus));
			  
		
			for(m =1; m < select->nb_enfants;m++){
			  //calcul bValue
			  if(etat->joueur != numJoueur)
			    bValue2 = (select->enfants[m]->nb_victoires/(double)select->enfants[m]->nb_simus) + (c* sqrt(log(select->enfants[m]->parent->nb_simus)/(double)select->enfants[m]->nb_simus));
			  else 
			    bValue2 = -(select->enfants[m]->nb_victoires/(double)select->enfants[m]->nb_simus) + (c* sqrt(log(select->enfants[m]->parent->nb_simus)/(double)select->enfants[m]->nb_simus));

			  //maj
			  if(bValue < bValue2){
			    bValue = bValue2;
			    indiceEnfant = m;
			  }
			  
			}
			select = select->enfants[indiceEnfant];
		      }
		}
		      	
		//maj du joueur courant
		numJoueur = AUTRE_JOUEUR(numJoueur);
		select->joueur = numJoueur;

		      
		} 
		
	    
	    //backpropagation
	    tmp = testFin(select->etat);
	    while(select->parent != NULL){
	      select->nb_simus += 1;
	      if( tmp == ORDI_GAGNE ){
		select->nb_victoires += 1;
	      }
	      /*if( tmp == HUMAIN_GAGNE ){
		select->nb_victoires -= 1;
	      }*/
	      select = select->parent;
	    }
	    racine->nb_simus++;
	    if(tmp == ORDI_GAGNE){
	      racine->nb_victoires++;
	    }
	    /*if( tmp == HUMAIN_GAGNE ){
		racine->nb_victoires -= 1;
	     }*/
	    
	    toc = clock(); 
	    temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
	    iter ++;
	} while ( temps < tempsmax );

	
	/* fin de l'algorithme  */ 
	
	// Jouer le meilleur coup
	double best = 0;
	for(i = 0;i < racine->nb_enfants;i++){
	  if(racine->enfants[i]->nb_simus != 0)
	    if(best < racine->enfants[i]->nb_victoires/(double)racine->enfants[i]->nb_simus){
	      best = racine->enfants[i]->nb_victoires/(double)racine->enfants[i]->nb_simus;
	      meilleur_coup = choix[i];
	    }
	}
		
	/* affichage nombre iteration et probabilité victorie : nombre victoire/simulation */
    printf("Nombre d'iteration : %d \n",iter);
    printf("Chance de victoire : %f \n",best);
	jouerCoup(etat, meilleur_coup );
	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
	free (choix);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;
	
	// initialisation
	Etat * etat = etat_initial(); 
	
	// Choisir qui commence : 
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );
	
	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);
		
		if ( etat->joueur == 0 ) {
			// tour de l'humain
			
			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );
									
		}
		else {
			// tour de l'Ordinateur
			
			ordijoue_mcts( etat, TEMPS );
			
		}
		
		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);
		
	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
