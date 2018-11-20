/*
Vivek Patel
UTA ID: 1001398338
*/


#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1

/* TODO */
/* Add your synchronization variables here */

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;  /*To take care professor break after every 10 students */
static int classa_total;         /*Total numbers of student from class A in office so far*/
static int classb_total;         /*Total numbers of student from class B in office so far*/
int classa_waiting = 0;   /*For being fair professsor need to give time to students from both sec*/
/*If students are waiting then after five students, professor need to see student from new class*/
int classb_waiting = 0;
/*To know the total number of student seen by professor and also helps in debugging*/
int students_visited_yet =0;
/*Variable used to take care of professors break*/ 
static int student_permitted_yet;

sem_t office_chairs;   /*Semaphore to take care of number of chairs in the office*/
pthread_mutex_t mutex; /*Mutex for blocking student to eneter the class*/
pthread_mutex_t mutex2; /*Mutex for blocking students to make changes in the global at a time*/
pthread_mutex_t mutex3; /*Mutex for checking the permission of the student*/ 
typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;
  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */
  classa_total = 0;
  classb_total = 0;
  student_permitted_yet = 0;
 
  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d\n", &(si[i].arrival_time), &(si[i].question_time))!=EOF) && i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
   sleep(5);
   assert( students_in_office == 0 );  
   int value;
   sem_getvalue(&office_chairs, &value);
   while(value){sem_wait(&office_chairs); sem_getvalue(&office_chairs, &value);}
   student_permitted_yet = 0;
   students_since_break = 0;
   int i;
   for(i = 0; i<3;i++){sem_post(&office_chairs);}
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
   printf("The professor arrived and is starting his office hours\n");
   /* Loop while waiting for students to arrive. */
   while (1) 
   {

     /* TODO */
     if(students_since_break >=10 && students_in_office == 0)
     {
        printf("\nTime to take break for professor.\n\n");
        take_break();
     }
     
     
     /* Add code here to handle the student's request.             */
     /* Currently the body of the loop is empty. There's           */
     /* no communication between professor and students, i.e. all  */
     /* students are admitted without regard of the number         */ 
     /* of available seats, which class a student is in,           */
     /* and whether the professor needs a break. You need to add   */
     /* all of this.                                               */

   }
   pthread_exit(NULL);
}

/*In the below function "Should Wiat" student will check whether some other
  student from different class is waiting and there has been 5 or more students seen in a row 
  of his own section. If it is true than function will return 1 which will then make 
  student to wait*/
static int should_wait(int class)
{
  pthread_mutex_lock(&mutex3);
  int k = class;
  if(k == CLASSA && classa_total >=5 && classb_waiting == 1 )
  { 
     pthread_mutex_unlock(&mutex3);
     return 1;
  }
  if(k == CLASSB && classb_total >=5 && classa_waiting == 1 )
  { 
     pthread_mutex_unlock(&mutex3);  
     return 1;
  }
  pthread_mutex_unlock(&mutex3);return 0;
}
/*Student_request give permission to the student whether to enter in the office or not.
  Threads are used because one student can aks for permission at a time*/
int student_request(int class)
{
   pthread_mutex_lock(&mutex); /*Using threads so that one student requesting at a time*/
   int permit = 0;
   int Class = class;
   if(Class == CLASSA && classb_inoffice!=0 && classa_waiting !=1)
   {
    /*Request denied for a student from class A since there is student from class B in the office, 
        and another student from class A requested before*/
      classa_waiting = 1;
      pthread_mutex_unlock(&mutex);
      return 0;
   }
   if(Class == CLASSB && classa_inoffice!=0 && classb_waiting !=1)
   {
     /*Request denied for a student from class A since there is student from class B in the office, 
        and another student from class A requested before*/
        classb_waiting = 1;
        pthread_mutex_unlock(&mutex);
        return 0;
   }
   /*Chceking whether other student from differention class is waiting */
   if(should_wait(Class))
   { 
        pthread_mutex_unlock(&mutex);
        return 0;
   }
   
   if(Class == CLASSA && classb_inoffice==0 )
   {
        permit = 1;
   }
  
   if(Class == CLASSB && classa_inoffice==0)
   {
        permit = 1;
   } 
  
   if(permit == 1)
   {    
        student_permitted_yet+=1;
        if(student_permitted_yet >10)
        {
                while(students_since_break){}
                pthread_mutex_unlock(&mutex);
                return 0;
        }  
        int value;
        sem_getvalue(&office_chairs, &value);
        
        if(!value)
        {     
             if(should_wait(class))
             {     
                   student_permitted_yet -=1;
                   pthread_mutex_unlock(&mutex);
                   return 0;
             }
        }
       
        sem_wait(&office_chairs);
        students_visited_yet++;
   /* Since student is permitted then increase the number of student in class a or b*/
  /*so that student from another class does not get permission*/
        if(Class == CLASSA){classa_inoffice +=1;}else {classb_inoffice +=1;}       
        printf("Th number of students since break is %d, and permitted is %d and visited is %d\n", students_since_break,student_permitted_yet, students_visited_yet);
        pthread_mutex_unlock(&mutex);
        return permit;
   }     
   pthread_mutex_unlock(&mutex);
   return permit;
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE. 
                                                      */ 
  while(!student_request(CLASSA)){}
  pthread_mutex_lock(&mutex2);
  classb_total = 0;
  classa_waiting = 0;
  students_in_office += 1;
  classa_total +=1;
  students_since_break += 1;
  pthread_mutex_unlock(&mutex2);
}


/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */ 
  
    while(!student_request(CLASSB)){}
    pthread_mutex_lock(&mutex2);
    classa_total = 0;
    classb_waiting = 0;
    students_in_office += 1;
    classb_total +=1;
    students_since_break += 1;      
    pthread_mutex_unlock(&mutex2);
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
  /* 
   *  TODO
   *  YOUR CODE HERE. 
   */
  pthread_mutex_lock(&mutex2);
  classa_inoffice -= 1;
  students_in_office -= 1;
  sem_post(&office_chairs);
  pthread_mutex_unlock(&mutex2);
 
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
  /* 
   * TODO
   * YOUR CODE HERE. 
   */
  //lock mutex so that one change at a time
  pthread_mutex_lock(&mutex2);
  classb_inoffice -= 1;
  students_in_office -= 1; 
  sem_post(&office_chairs);
  pthread_mutex_unlock(&mutex2);
  
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  
  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  
  pthread_exit(NULL);
  
}

/* Main function sets up simulation and prints report
 * at the end.
 */
int main(int nargs, char **args) 
{
  int j = 2;
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];
  
  /*initialize semaphore which will take caree of the number of chairs in the office*/
  sem_init(&office_chairs, 0, MAX_SEATS);  
  /*initialize mutex to handle students to get into the */ 
  pthread_mutex_init( & mutex, NULL );
  pthread_mutex_init( & mutex2, NULL );
  pthread_mutex_init( & mutex3, NULL );
  
  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }
  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, (void *) &j);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    student_type = random() % 2;

    if (student_type == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);
  //pthread_cancel(&mutex);
  //pthread_cancel(&mutex2);
  //pthread_cancel(&mutex3);
  printf("Office hour simulation done.\n");

  return 0;
}
