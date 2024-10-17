## Riddle - Notes

### Challenge 0: 'Hello there'

* `open(".hello_there", 0, 00)`: FAIL<br>
Ανοίγει το αρχείο ".hello_there" ως O_RDONLY αγνοώντας τα permissions. Το αρχείο δεν υπάρχει οπότε το φτιάχνουμε 
	* `touch .hello_there`

### Challenge 1: 'Gatekeeper'
* `open(".hello_there", O_WRONLY, 00)`: FAIL
Οι υποδείξεις του STDERR μας κάνουν να υποθέσουμε ότι το πρόγραμμα κάνει fail αν καταφέρει να ανοίξει για write το αρχείο.
	* `chmod -w .hello_there`

### Challenge 2: 'A time to kill'
* `rt_sigaction(SIGALRM, {sa_handler=0x56228a714350, sa_mask=[ALRM], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f2f8fd2ad60}, {sa_handler=SIG_DFL, sa_mask=[], sa_flags=0}, 8) = 0`
* `rt_sigaction(SIGCONT, {sa_handler=0x56228a714350, sa_mask=[CONT], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f2f8fd2ad60}, {sa_handler=SIG_DFL, sa_mask=[], sa_flags=0}, 8) = 0`
* `alarm(10)                               = 0`
* `pause()                                 = ? ERESTARTNOHAND (To be restarted if no handler)`
* `--- SIGALRM {si_signo=SIGALRM, si_code=SI_KERNEL} ---`
* `rt_sigreturn({mask=[]})                 = -1 EINTR (Interrupted system call)`<br>
Η διεργασία περιμένει 10 δευτερόλεπτα και μέσω της υπόδειξης help me move on καταλαβαίνουμε ότι πρέπει να στείλουμε σήμα SIGCONT για να συνεχίσει η διεργασία
	* α' τρόπος: `ps -e | grep riddle`
	`kill -SIGCONT <pid>`
	* β' τρόπος: `Ctrl+C` + `fg`

### Challenge 3: 'what is the answer to life the universe and everything'
Από το αποτέλεσμα του ltrace:
* `getenv("ANSWER")`<br>	
	* export ANSWER=42

### Challenge 4: 'First-in, First-out'
* `open("magic_mirror", O_RDWR, S_IRUSR) = -1 ENOENT (No such file or directory)`
	* `touch magic_mirror`
* Παρατηρούμε ότι το αρχείο γράφει έναν χαρακτήρα στο magic_mirror και μετά πάει να διαβάσει έναν χαρακτήρα από αυτό, όμως αποτυγχάνει.<br>
Λαμβάνοντας υπόψιν και το όνομα του challenge, καταλαβαίνουμε πως πρέπει το magic_mirror να είναι τύπου FIFO ώστε να διαβάζει τον χαρακτήρα που μόλις έγραψε.
	* `rm magic_mirror`
	* mkfifo magic_mirror

### Challenge 5: 'my favourite fd is 99'
* `fcntl(99, F_GETFD)`<br>
Δεν υπάρχει ανοιχτός file descriptor 99 οπότε τον δημιουργούμε.
	* α' τρόπος: `touch challenge5.txt && exec 99<challenge5.txt`
	* β' τρόπος: Γράφουμε κώδικα σε C:
		```c
		#include <unistd.h>
		#include <fcntl.h>

		int main() {
			int fd = open("challenge5.txt", O_RDONLY);
			dup2(fd, 99);
			char *args[] = {"./riddle", NULL};
			execv("./riddle", args);
		}
		```
		και τον εκτελούμε.

### Challenge 6: 'ping pong'
* Παρατηρούμε ότι γίνονται δύο clone.
	* `strace -f` για να δούμε τι καλούν και τα δύο παιδιά.
* Ανταλλάσσουν PING PONG μέσω pipes 33, 34 που δεν υπάρχουν, οπότε τα φτιάχνουμε.
	* ```c
		#include <unistd.h>
		#include <fcntl.h>

		int main() {
			int fds1[2];
			pipe(fds1);
			dup2(fds1[0], 33);
			dup2(fds1[1], 34);

			char *args[] = {"./riddle", NULL};
			execv("./riddle", args);
		}
		```
* Παρατηρούμε ότι χρειάζονται επιπλέον τα pipes 53, 54, οπότε τα φτιάχνουμε.
	* ```c
		#include <unistd.h>
		#include <fcntl.h>

		int main() {
			int fds1[2];
			pipe(fds1);
			dup2(fds1[0], 33);
			dup2(fds1[1], 34);
			int fds2[2];
			pipe(fds2);
			dup2(fds2[0], 53);
			dup2(fds2[1], 54);

			char *args[] = {"./riddle", NULL};
			execv("./riddle", args);
		}
		```

### Challenge 7: 'What's in a name?'
* Παρατηρούμε ότι ανοίγουν δύο αρχεία `.hello_there` που υπήρχε από το Challenge 0 και το `.hey_there` που δεν υπήρχε, οπότε το φτιάξαμε.
	* `touch .hey_there`
* Παρατηρούμε ότι πάλι γίνεται FAIL με μήνυμα 786 != 1423. Σκεφτόμαστε τι μπορεί να σημαίνει αυτό, και λόγω και του ονόματος του challenge και το hint "a rose by any other name" σκεφτήκαμε ότι μάλλον συγκρίνει τα inodes, οπότε διαγράψαμε το αρχείο και το ξαναφτιάξαμε ως hard link στο `.hello_there`.
	* `rm .hey_there`
	* `ln .hello_there .hey_there`

### Challenge 8: Big Data
* Παρατηρούμε πως ανοίγει αρχείο "bf00" που δεν υπάρχει.
	* `touch bf00`
* Παρατηρούμε ότι κάνει lseek στη θέση 1073741824 και προσπαθεί να διαβάσει 16 χαρακτήρες ενώ γράφει κάποια `Χ`. Δεδομένου και του hint για footers καταλαβαίνουμε πως δεν χρειάζεται να φτιάξουμε όντως ένα γιγαντιαίο αρχείο αλλά αρκεί να γράψουμε τους 16 χαρακτήρες στο τέλος του στο σημείο του lseek. Διαγράφουμε το αρχείο bf00 με `rm bf00` και εκτελούμε τον παρακάτω κώδικα (που συνεχίζει και για τα υπόλοιπα απαιτούμενα αρχεία).
	* ```c
		#include <unistd.h>
		#include <fcntl.h>

		int main() {
			int fd0 = open("bf00", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd0, 1073741824, SEEK_SET);
			write(fd0, "XXXXXXXXXXXXXXXX", 16);

			int fd1 = open("bf01", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd1, 1073741824, SEEK_SET);
			write(fd1, "XXXXXXXXXXXXXXXX", 16);

			int fd2 = open("bf02", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd2, 1073741824, SEEK_SET);
			write(fd2, "XXXXXXXXXXXXXXXX", 16);

			int fd3 = open("bf03", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd3, 1073741824, SEEK_SET);
			write(fd3, "XXXXXXXXXXXXXXXX", 16);

			int fd4 = open("bf04", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd4, 1073741824, SEEK_SET);
			write(fd4, "XXXXXXXXXXXXXXXX", 16);

			int fd5 = open("bf05", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd5, 1073741824, SEEK_SET);
			write(fd5, "XXXXXXXXXXXXXXXX", 16);

			int fd6 = open("bf06", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd6, 1073741824, SEEK_SET);
			write(fd6, "XXXXXXXXXXXXXXXX", 16);

			int fd7 = open("bf07", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd7, 1073741824, SEEK_SET);
			write(fd7, "XXXXXXXXXXXXXXXX", 16);

			int fd8 = open("bf08", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd8, 1073741824, SEEK_SET);
			write(fd8, "XXXXXXXXXXXXXXXX", 16);

			int fd9 = open("bf09", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			lseek(fd9, 1073741824, SEEK_SET);
			write(fd9, "XXXXXXXXXXXXXXXX", 16);



			close(fd0);
			close(fd1);
			close(fd2);
			close(fd3);
			close(fd4);
			close(fd5);
			close(fd6);
			close(fd7);
			close(fd8);
			close(fd9);
		}
		```

### Challenge 9: 'Connect'
* Παρατηρούμε ότι ο κώδικας εκτελεί εντολή socket για σύνδεση IPv4 και connect για την σύνδεση του file descriptor που επέστρεψε το socket σε συγκεκριμένη IP address και port, η οποία αποτυγχάνει. Από τη λέξη listen στο μήνυμα του κωδικα καταλάβαμε ότι πρέπει να κάνουμε τον υπολογιστή να ακούει την συγκεκριμένη IP address στο συγκεκριμένο port.
	* `nc -l -s 127.0.0.1 -p 49842`
* Εμφανίστηκε στην οθόνη μία ερώτηση για ένα άθροισμα για επιβεβαίωση, το οποίο απαντήσαμε και ο κώδικας πέτυχε.

### Challenge 10: 'ESP'
* Ο κώδικας άνοιγε (και δημιουργούσε αν δεν υπήρχε) ένα αρχείο secret_number, έγραφε τον μυστικό αριθμίο και μετά έκανε unlink το αρχείο. Αν υπήρχε και άλλο link στο αρχείο, θα μπορούσαμε να διατηρήσουμε πρόσβαση σε αυτό.
	* `touch secret_number`
	* `ln secret_number secret_number_link`
Τρέχουμε το riddle, κάνουμε `cat secret_number_link` και παίρνουμε τον αριθμό.

### Challenge 11: 'ESP-2'
