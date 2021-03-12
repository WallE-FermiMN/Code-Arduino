#include <stdio.h>
#include <termios.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

int init(char *comname, int mode);       // Funzione che apre una seriale e la inizializza
#define   VIRTUALTTY
#define TYPE 0
#define TERMINATORECRU 4
#define DATI 1
#define CHECKSUM 2
#define TERMINATORENOR 3

#define SERIALE "/dev/ttyUSB0"
int main()
{
    int errread;
    int com = init(SERIALE,O_RDWR);        // Apre il canale seriale sia in ingresso sia in uscita
    if(com>0){
        do {
            //read(com,&car,1);
            if ((errread = read(com, &car, 1)) < 1) { //read(seriale,puntatore,lunghezza)
                printf("File chiuso dall'altro estermo o errore di lettura - %d\n", errread);
                if (errread < 0) {
                    perror("Errore di lettura: ");
                }
                break;
            }
        }while
    close(com);
    return 0;
}

/* Funzione che crea una finestra e le disegna una cornice intorno
 * La finestra ha altezza 'h', larghezza 'w', e' posizionata ad 'x', 'y'.
 * La cornice e' esterna alla finestra (crea una finestra temporanea per disegnarla)
 * La cornice e' gialla su sfondo blu.
 * La finesra e' blu, con testo bianco.
 */
/* ----  Blocco di gestione della seriale (reale o virtuale) --- */
#ifdef VIRTUALTTY
#include <errno.h>
#include <signal.h>
int grantpt(int fd);
int unlockpt(int fd);
char *ptsname(int fd);
void __finish(){
    unlink("/tmp/ttyLOCAL.ctl");
}
int init (char *comname, int mode) {
    const char *ttyControlFile = "/tmp/ttyLOCAL.ctl";
    const char *ttyControlFormat = "%d %s\n";
    struct termios tattr;                // Struttura per impostare le caratteristiche della seriale
    int com = -1;
    FILE *control = fopen(ttyControlFile, "r");

    if (control) {
        int pid;
        char slave[30];
        if (fscanf(control, ttyControlFormat, &pid, slave) == 2) {
            char pidname [15];
            int pt;
            sprintf (pidname, "/proc/%d", pid);
            if ((pt = open(pidname, O_RDONLY)) >= 0){
                com = open (slave, mode | O_SYNC);
            }
        }
        fclose (control);
    }
    if (com < 0) {
        com = open("/dev/ptmx", mode | O_SYNC);
        if (com >= 0) {
            control = fopen(ttyControlFile, "w");
            if (control) {
                atexit(__finish);
                signal(SIGINT, __finish);
                grantpt(com);
                unlockpt(com);
                fprintf(control, ttyControlFormat, getpid(), ptsname(com));
                fclose (control);
            } else {
                perror ("Non riesco ad creare il file di controllo");
                close (com);
                exit (2);
            }
        } else {
            perror ("Non riesco ad creare il file master della seriale virtuale");
            exit (1);
        }
    }
    /* Impostazione della seriale
     * fonte info libc->"Low-Level Terminal Interface"->"Noncanonical Mode Example" */
    tcgetattr (com, &tattr);                // Recupera gli attributi correnti della seriale

    // Modi di ingresso - spegne i flag indicati
    tattr.c_iflag &= ~(INLCR|IGNCR|ICRNL|IXON);
    // Modi di uscita - spegne i flag indicati
    tattr.c_oflag &= ~(OPOST|ONLCR|OCRNL|ONLRET);
    /* Modi di controllo - imposta 8 bit (CS8), abilita ricezione (CREAD),
     *ignora segnali di controllo (CLOCAL) */
    tattr.c_cflag = CS8 | CREAD | CLOCAL;
    tattr.c_lflag &= ~(ICANON|ECHO);        // elimina traduzioni di carateri (ICANON) ed ECHO.
    tattr.c_cc[VMIN] = 1;                   // restituisce dati dopo aver ricevuto 1 carattere
    tattr.c_cc[VTIME] = 0;                  // nessun timeout

    tcsetattr (com, TCSAFLUSH, &tattr);        // Imposta i nuovo attributi

    return com;
}
#else
int init(char *comname, int mode)
{
int com;                        // Il file per la seriale
struct termios tattr;                // Struttura per impostare le caratteristiche della seriale

  com = open (comname,mode | O_SYNC);        // Apre il dispositivo (Lettura/scrittura, sincrono)

  if (com == -1)                        // Open ritorna errore
  {
    perror ("Non posso aprire la seriale"); // Stampa il messaggio di errore
    exit (2);                                // E termina drasticamente con errore
  }

  /* Impostazione della seriale
   * fonte info libc->"Low-Level Terminal Interface"->"Noncanonical Mode Example" */
  tcgetattr (com, &tattr);                // Recupera gli attributi correnti della seriale

  // Modi di ingresso - spegne i flag indicati
  tattr.c_iflag &= ~(INLCR|IGNCR|ICRNL|IXON);
  // Modi di uscita - spegne i flag indicati
  tattr.c_oflag &= ~(OPOST|ONLCR|OCRNL|ONLRET);
  /* Modi di controllo - imposta 8 bit (CS8), abilita ricezione (CREAD),
   *ignora segnali di controllo (CLOCAL) */
  tattr.c_cflag = CS8 | CREAD | CLOCAL;
  tattr.c_lflag &= ~(ICANON|ECHO);        // elimina traduzioni di carateri (ICANON) ed ECHO.
  tattr.c_cc[VMIN] = 1;                        // restituisce dati dopo aver ricevuto 1 carattere
  tattr.c_cc[VTIME] = 0;                // nessun timeout

  cfsetispeed (&tattr, B9600);                // Imposta la velocita' di ricezione
  cfsetospeed (&tattr, B9600);                // Imposta la velocita' di trasmissione

  tcsetattr (com, TCSAFLUSH, &tattr);        // Imposta i nuovo attributi

  return com;                                // Ritrona l'handle del file aperto
}
#endif
/* ----  Fine blocco di gestione della seriale (reale o virtuale) --- */
