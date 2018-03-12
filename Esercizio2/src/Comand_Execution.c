#include <stdio.h>  /* Libreria che contiene operazioni di input/output */
#include <fcntl.h>  /* Libreria che contiene opzioni di controllo dei file */
#include <string.h> /* Libreria che contiene operazione di manipolazioni stringhe e memoria */
#include <stdlib.h> /* Libreria che dichiara funzioni e costanti di utilità generale */
#include <unistd.h> /* Libreria che consente l'accesso alle API dello standard POSIX */
#include <errno.h>  /* Libreria che contiene definizioni di macro per la gestione delle situazioni di errore */
#include <signal.h> /* Libreria che contiene macro per gestire segnali diversi riportati durante l'esecuzione di un programma */
#include <sys/wait.h> /* Libreria che contiene opzioni di controllo della wait() */
#define BUFFER 1024 /* Grandezza del buffer nel quale inserire l'input da tastiera */

int num_file = 1; /* Inizializzo il numero del file di output */
char newline[1] = "\n"; /* Definisco un vettore contenente carattere di nuova linea */

/* execution() effettua l'esecuzione del comando inserito dall'utente sottoforma di input,
controllando se devono essere svolti sequenzialmente o no, tramite una wait(), se ci sono eventuali errori,
e infine chiude il file di output specifico */

void execution(char *ar[], int f, char control){

	printf("\n\nSoluzione :\n\n");

	if(fork() != 0){  /* Viene creato un processo figlio per ogni comando inserito*/

		if(control == 's'){ /*  Se i comandi devono essere eseguiti sequenzialmente utilizzo una wait() affinche il processo padre attenda la terminazione del processo 					figlio prima di passare al sucessivo processo figlio, invece se i comandi devono essere svolti in parallelo , anche se un processo figlio è 						lungo , se arriva nel frattempo un secondo comando, esso viene eseguito in parallelo con un altro processo figlio, in quanto non si utilizza 						nessuna wait() come controllo sulla terminazione o meno del processo figlio */

			wait(NULL);

		}

	}else{


		int fpid = getpid(); /* Restituisce l' ID del processo figlio del comando */

		int er = execvp(ar[0], &ar[0]); /* Viene eseguito il comando con i rispettivi parametri e restituisci -1 se il comando è errato */

		if(er == -1){

			printf("Errore = %d\n\n", errno); /* La variabile errno contiene il numero dell' errore commesso all' esecuzione del comando ch enon è andato a buon 									  fine*/

			char *p=strerror(errno); /* Puntatore alla stringa di errore che viene restituita dalla funzione*/

			write(f, p, strlen(p)); /* Scrivo l'errore sul file di out.i specifico*/

			kill(fpid, SIGKILL); /* Invia al processo figlio il segnale SIGKILL cioè il processo viene terminato in maniera sicura  */
		}

		close(f); /* Il file di output del comando viene chiuso in quanto compilato*/
	}

}

/* change_sdoutput() effettua la creazioend del file specifico di output e gli assegna il file_descriptor, il quale viene restituito come ritorno */

int change_sdoutput(){

	int fd1;
	int fd2;


	char out[6]; /* Vettore contenente il nome del file output */
	sprintf(out, "out.%d", num_file); /* Creazione e inserimento del nome del file output nel vettore */

	remove(out); /* Viene azzerato il contenuto del file con quel nome se già esistente */

	fd1 = open(out, O_CREAT | O_RDWR, 0666); /* Viene effettuata l'apertura del file , se non esiste si crea e
						gli si assegna i permessi di lettura e scrittura al proprietario e al gruppo */

	num_file++; /* Si incrementa il numero che indentifica il file */

	close (1); /* Viene effettuata la chiusura del file di output definito dal sitema */

	fd2 = dup (fd1); /* Creo una copia del descrittore di file fd1, utilizzando il descrittore di file inutilizzato con il numero più basso,
			 che è quello di ouput, per il nuovo descrittore. */

	return fd2; /* Valore di ritorno è il descrittore di file di output del comando  */
}

/* comand_seq() elabora in maniera sequenzale ogni riga di comanda inserita dall'utente */

int comand_seq(){

	char acr[BUFFER]; /* Buffer nel quale inserire la riga di comando */
	fgets(acr, BUFFER, stdin); /* Lettura da tasiera della riga di comando la quale viene salvata nel buffer */

	acr[strlen(acr)-1] = '\0'; /* fgets() inserisce al penultimo valore "\n", ma per l'operazione successiva ho bisogno che tale valore sia "\0" */

	if(strlen(acr) != 0){ /* Controllo se la lunghezza della riga inserita è uguale o diversa da zero*/

		int f = change_sdoutput();

		const char s[2]= " "; /* Vettore contenente il carattere di separazione */
		char *token; /* Puntatore usato per l'ultimo token della lista  */
		char *ar[BUFFER]; /* Buffer contente la riga comando rivisionata */
		token = strtok(acr, s); /* Assegna al puntatore il primo token */

		printf("Comando Inserito :\n\n");

		int i=0;

		while(token != NULL){ /* Finche il puntatore non punta a nulla esegue la ricerca del successivo token */

			ar[i]=token;
			printf("%s ", token);
			token=strtok(NULL,s);
			i++;
		}
		ar[i]=NULL; /* L'ultimo valore di un vettore che contiene un comando eseguibile deve essere NULL */

		execution(ar, f, 's'); /* Esegue il comando inserito nel vettore ar, passandogli anche il file di output */

		return 0; /* Indica che il comando c'è, che sia esatto o meno */

	}else{

		return 1; /* Indica che è stato inserito un nuova linea vuota */

	}
}

/* sequenziale() cicla finche l'utente non inserisce una riga vuota, altrimenti esegue il comando inserito. Vedremo che qui non si usa nessun file intermedio,
ciò rende piu l'idea di sequenzialità, oltre a utilizzare nella execution() la funzione wait() che ci permette lo svolgimento sequenziale dei comandi  */

void sequenziale(){

	printf("Inserisci i comandi da svolgere in modo sequenziale : \n");

	while(1){ /* Cicla finche non viene inserita una riga vuota */

		int control = comand_seq();

		if(control == 1){

			exit(1); /* Indica l'uscita dal programma a causa di inserimento di riga vuota */
		}
	}
}

/* comand_par() elabora in maniera "parallela" ogni riga di comanda inserita dall'utente, tramite l'uso di un file intermedio */

void comand_par(){

	FILE *fd; /* Puntatore al file che conterra tutti i comandi che sono stati inseriti precedentemente dall'utente*/
  	char buf[BUFFER]; /* Buffer nel quale inserire la riga di comando letta dal file dei comandi */
  	char *res; /* Puntatore al buffer che conterrà la riga di comando letta dal file dei comandi */

 	fd=fopen("file_comand.txt", "r");

  	if( fd==NULL ){

   		perror("Errore in apertura del file"); /* Vuol dire che il file non si è aperto in maniera corretta quindi si esce dal programma*/
    		exit(1);

  	}

  	while(1){ /* Cicla finché non viene letta dal file una riga vuota */

    		res=fgets(buf, BUFFER, fd); /* Restituisce il puntatore al buffer contenente la riga di comanda*/

    		if( res==NULL ){
     			break; /* Esce dal while in quanto è stata inserita una riga vuota nel file dei comandi, quindi punta a NULL */
		}

		int f = change_sdoutput();

		/* Stesso procedimento di creazione del vettore comando, di comand_seq() */

		buf[strlen(buf)-1] = '\0';

		const char s[2]= " ";
		char *token;
		char *ar[BUFFER];
		token = strtok(buf, s);

		printf("Comando Inserito :\n\n");

		int i=0;

		while(token != NULL){

			ar[i]=token;
			printf("%s ", token);
			token=strtok(NULL,s);
			i++;
		}

		ar[i]=NULL; /* L'ultimo valore di un vettore che contiene un comando eseguibile deve essere NULL */

		execution(ar, f, 'p'); /* Esegue il comando inserito nel vettore ar, passandogli anche il file di output */
	}

 	fclose(fd); /* Il file dei comandi viene chiuso */
}

/* parallelo() cicla finche l'utente non inserisce una riga vuota, salvando ogni comando in un file dei comandi per poi eseguirlo in "parallelo".
Nel parallelo usiamo un file d'appagio affinche ciò renda l'idea di un potenziale parallelismo sulla creazione del file, oltre al fatto che come
vedremo nella execution(), per il parallelo per come lo abbiamo implementato , non effetua nessuna attesa della terminazione di un processo figlio
prima di crearne un altro da parte del processo padre */

void parallelo(){


	printf("Inserisci i comandi da svolgere in modo parallelo : \n");

	remove("file_comand.txt"); /* Viene azzerato il contenuto del file dei comandi se già esistente */

	int fdc = open("file_comand.txt", O_CREAT | O_RDWR , 0666); /* Viene effettuata l'apertura del file dei comandi, se non esiste si crea e
								    gli si assegna i permessi di lettura e scrittura al proprietario e al gruppo */

	int control = 0; /* Variabile di controllo sul ciclo */

	while(control == 0){ /* Cicla finchè l'utente non inserisce una riga vuota*/

		char arcom[BUFFER]; /* Buffer contente la riga di comando */
		fgets(arcom, BUFFER, stdin); /* Lettura da tasiera della riga di comando la quale viene salvata nel buffer */

		arcom[strlen(arcom)-1] = '\0'; /* fgets() inserisce al penultimo valore "\n", ma per l'operazione successiva ho bisogno che tale valore sia "\0" */

		if(strlen(arcom) == 0){ /* Controllo sulla lunghezza del buffer */

			control = 1; /* Vuol dire che la riga è vuota */

		}else{

			write(fdc, arcom, strlen(arcom)); /* Inserisco la riga scritta su terminale nel file dei comandi */
			write(fdc, newline, strlen(newline)); /* Inserisco il valore di newline */
		}
	}

	close(fdc); /* Chiudo il file dei comandi */

	comand_par(); /* Richiamo il metodo per far eseguire in "parallelo" i comandi inseriti nel file dei comandi */

	exit(1); /* Indica l'uscita dal programma a causa della lettura di tutti i comandi  */
}

/* main() avvia il programma stampando a video il menù con la scelta effettiabile dall' utente e prende in input un valore che identifica la modalità
di esecuzione dei comandi successivamente inseriti "s" = sequenziale e "p" = parallelo */

void main(){

	printf("Seleziona 's' se vuoi eseguire una serie di comandi in modo sequenziale\n");
	printf("Seleziona 'p' se vuoi eseguire una serie di comandi in modo paralello\n");

	/* La scanf lascia nel buffer un carattere di fine riga '\n'. Quando chiami getchar(), in pratica toglie questo carattere dal buffer facendo si che la fgets 		    		funzioni senza problemi. Senza il getchar(), la fgets legge come primo carattere un '\n' e si interrompe subito  */

	char sp[1024];
	char *p = sp;

	scanf("%s", p);
	getchar();

	if(strlen(sp) == 1){

		switch(sp[0]){ /* Esegue lo switch sul valore inserito */

			case 's': sequenziale(); /* Esegue i comandi inseriti da tastiera in modo sequenziale */
				  break;

			case 'p': parallelo(); /* Esegue i comandi inseriti da tastiera in modo "parallelo" */
				  break;

			default : printf("Riprova\n");
				  main(); /* Richiama il metodo main() in quanto il dato inserito non è tra quelli indicati */

		}

	}else{

		printf("Riprova\n");
		main(); /* Richiama il metodo main() in quanto il dato inserito non è tra quelli indicati */
	}

}

