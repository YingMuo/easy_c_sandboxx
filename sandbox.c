#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

int main( int argc, char *argv[] )
{
    char *program = argv[1];
    char *program_argv[] = { program, 0 };
    char *file_input = argv[2];
    char *file_output = argv[3];
    char *file_error = argv[4];
    char *file_result = argv[5];

    long time_limit = strtol( argv[6], NULL, 10 );
    long memory_limit = strtol( argv[7], NULL, 10 );
    long output_limit = strtol( argv[8], NULL, 10 );
    long process_limit = strtol( argv[9], NULL, 10 );

    #ifdef DEBUG
        printf( "\tprogram: %s\n", program );
        printf( "\tfile_input: %s\n", file_input );
        printf( "\tfile_output: %s\n", file_output );
        printf( "\tfile_error: %s\n", file_error );
        printf( "\tfile_result: %s\n", file_result );
        printf( "\ttime_limit: %ld\n", time_limit );
        printf( "\tmemory_limit: %ld\n", memory_limit );
        printf( "\toutput_limit: %ld\n", output_limit );
        printf( "\tprocess_limit: %ld\n", process_limit );    
    #endif

    pid_t pid = fork();

    // fork error
    if( pid == -1 )
    {
        perror( "fork" );
        exit( EXIT_FAILURE );
    }

    // parent process
    else if( pid > 0 )
    {
        // wait for child process
        siginfo_t child_siginfo;
        if( waitid( P_PID, pid, &child_siginfo, WEXITED | WSTOPPED ) == -1 )
        {
            perror( "waitid" );
            exit( EXIT_FAILURE );
        }

        // open file_result
        FILE *result = fopen( file_result, "w" );
        if( result == NULL )
        {
            perror( "fopen" );
        }

        // get resource usage to child
        struct rusage usage;
        if( getrusage( RUSAGE_CHILDREN, &usage ) == -1 )
        {
            perror( "getrusage" );
            exit( EXIT_FAILURE );
        }

        double used_time = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000;

        fprintf( result, "----------------resource----------------\n" );
        fprintf( result, "used memory: %ld\n", usage.ru_maxrss );
        fprintf( result, "used time: %lf\n", used_time );
        fprintf( result, "------------------end-------------------\n" );

        // limit judge
        if( child_siginfo.si_code == CLD_EXITED )
        {
            if( child_siginfo.si_status == 0 )
            {
                fprintf( result, "execute normally.\n" );;
            }
            else
            {
                fprintf( result, "RE\n\tsignal: %s\n", strsignal( child_siginfo.si_status ) );
            }

            fprintf( result, "\texit status: %d\n", child_siginfo.si_status );
        }
        else
        {
            if( usage.ru_utime.tv_sec > time_limit || child_siginfo.si_status == SIGKILL || child_siginfo.si_status == SIGXCPU )
            {
                fprintf( result, "TLE\n" );
            }
            else if( usage.ru_maxrss > memory_limit )
            {
                fprintf( result, "MLE\n" );
            }
            else if( child_siginfo.si_status == SIGXFSZ )
            {
                fprintf( result, "OLE\n" );
            }
            else
            {
                fprintf( result, "RE\n");
            }

            fprintf( result, "\tsignal: %s\n", strsignal( child_siginfo.si_status ) );
        }

        fclose( result );
    }

    // child process
    else
    {
        // set some limit
        if( time_limit )
        {
            struct rlimit lim;
            lim.rlim_cur = time_limit;
            lim.rlim_max = time_limit + 1;
            setrlimit( RLIMIT_CPU, &lim );
        }

        if( memory_limit )
        {
            struct rlimit lim;
            lim.rlim_cur = memory_limit;
            lim.rlim_max = memory_limit + 1024;
            setrlimit( RLIMIT_AS, &lim );
        }

        if( output_limit )
        {
            struct rlimit lim;
            lim.rlim_cur = output_limit;
            lim.rlim_max = output_limit + 1;
            setrlimit( RLIMIT_FSIZE, &lim );
        }

        if( process_limit )
        {
            struct rlimit lim;
            lim.rlim_cur = process_limit;
            lim.rlim_max = process_limit + 1;
            setrlimit( RLIMIT_NPROC, &lim );
        }

        // change standard file desciptor
        int fd = 0;

        fd = open( file_input, O_RDONLY );
        dup2( fd, STDIN_FILENO );
        close( fd );

        fd  = open( file_output, O_WRONLY );
        dup2( fd, STDOUT_FILENO );
        close( fd );

        fd = open( file_error, O_WRONLY );
        dup2( fd, STDERR_FILENO );
        close( fd );

        execvp( program, program_argv );
    }

    return 0;
}