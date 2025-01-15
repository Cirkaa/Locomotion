#include <iostream>
#include "windows.h"
#include "stdio.h"
#include "lomo2.h"
#include "stdlib.h"
#include "winuser.h"//para el temporizador 
#define MAX 100
#define MAX2 400

int(*LOMO_generar_mapa)(const CHAR*, const CHAR*);
int(*LOMO_inicio)(int, int, const char*, const char*);
int(*LOMO_trenNuevo)(void);
int(*LOMO_peticiOnAvance)(int, int*, int*);
int(*LOMO_avance)(int, int*, int*);
char* (*LOMO_getColor)(int);
void(*LOMO_espera)(int, int);
int(*LOMO_fin)(void);
void(*pon_error)(const char*);

//Globales
int mapa[17][75];
int vectorIdTrenes[MAX];
int nTrenes;

HANDLE vectorTrenes[MAX];
HANDLE trenPadre;
HANDLE sem_EsperarPadre;
HANDLE semaforos[MAX2];

HINSTANCE libreria;


/*######### PROTOTIPOS #############*/
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
void terminarPrograma(int signal);
DWORD WINAPI funcionTrenesHijos(LPVOID param);


int main(int argc, char* argv[]) {
    int i;
    int ret = 0;
    int tamMaximo = 0;

    trenPadre = GetCurrentThread();    //identificamos el padre

    ///////////////////////////////////////////////////////////////////
    ///     	CAMBIO DE LA SENIAL SIGINT A LA MANEJADORA	
    ///////////////////////////////////////////////////////////////////
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        printf("\nERROR: no se ha podido cambiar el funcionamiento de SIGINT.\n\n");
        terminarPrograma(1);
        exit(1);
    }


    //////////////////////////////////////////////////////////////////
    ///             	VINCULACION DE LA DLL
    //////////////////////////////////////////////////////////////////
    libreria = LoadLibrary(TEXT("lomo2.dll"));
    if (libreria == NULL) {
        fprintf(stderr, "No se ha podido cargar la biblioteca\n");
        terminarPrograma(1);
        return 100;
    }

    LOMO_generar_mapa = (int (*) (const char*, const char*)) GetProcAddress(libreria, "LOMO_generar_mapa");
    if (LOMO_generar_mapa == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_generar_mapa\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_inicio = (int (*) (int, int, const char*, const char*)) GetProcAddress(libreria, "LOMO_inicio");
    if (LOMO_inicio == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_inicio\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_trenNuevo = (int(*)(void)) GetProcAddress(libreria, "LOMO_trenNuevo");
    if (LOMO_trenNuevo == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_trenNuevo\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_peticiOnAvance = (int (*) (int, int*, int*)) GetProcAddress(libreria, "LOMO_peticiOnAvance");
    if (LOMO_peticiOnAvance == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_peticiOnAvance\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_avance = (int (*) (int, int*, int*)) GetProcAddress(libreria, "LOMO_avance");
    if (LOMO_avance == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_avance\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_getColor = (char* (*) (int)) GetProcAddress(libreria, "LOMO_getColor"); //no estoy seguro
    if (LOMO_getColor == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_getColor\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_espera = (void (*) (int, int)) GetProcAddress(libreria, "LOMO_espera");
    if (LOMO_espera == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_espera\n");
        terminarPrograma(1);
        return 100;
    }
    LOMO_fin = (int (*) (void)) GetProcAddress(libreria, "LOMO_fin");
    if (LOMO_fin == NULL) {
        fprintf(stderr, "No se ha podido cargar LOMO_fin\n");
        terminarPrograma(1);
        return 100;
    }
    pon_error = (void (*) (const char*)) GetProcAddress(libreria, "pon_error");
    if (pon_error == NULL) {
        fprintf(stderr, "No se ha podido cargar pon_error\n");
        terminarPrograma(1);
        return 100;
    }


    //////////////////////////////////////////////////////////////////
    ///             	COMPROBACIONES INICIALES	
    //////////////////////////////////////////////////////////////////
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Debe introducir:\n lomo2 --mapa: si quiere ver el mapa\n lomo2 retardo tamMaximo nTrenes: para ver el funcionamiento de la practica\n");
        exit(1);
    }
    //primer caso, si solo introduce un argumento despues de ./lomo
    else if (argc == 2) {
        if (strcmp(argv[1], "--mapa") == 0) { //generamos mapa
            if (LOMO_generar_mapa("i0546315", "i0939352") == -1) {
                perror("ERROR, problema al llamar LOMO_generar_Mapa\n");
                terminarPrograma(1);
                return 100;
            }
        }
        else {
            //si no introduce ni uno ni dos argumentos despues de ./lomo
            fprintf(stderr, "Debe introducir:\n lomo2 --mapa: si quiere ver el mapa\n lomo retardo tamMaximo nTrenes: para ver el funcionamiento de la practica\n");
            terminarPrograma(1);
            exit(1);
        }

    }
    //segundo caso, si introduce tres argumentos despues de ./lomo
    //lomo2 retardo tamMAximo nTrenes
    else if (argc == 4) {
        ret = atoi(argv[1]);
        if (ret < 0) {
            fprintf(stderr, "El primer parametro, retardo, debe ser mayor o igual que 0.\n");
            terminarPrograma(1);
            exit(1);
        }
        tamMaximo = atoi(argv[2]);
        if (tamMaximo < 3 || tamMaximo>20) {
            fprintf(stderr, "El segundo parametro, tamMaximo, debe ser mayor que tres y menor que 20.\n");
            terminarPrograma(1);
            exit(1);
        }
        nTrenes = atoi(argv[3]);
        if (nTrenes < 0 || nTrenes>100) {
            fprintf(stderr, "El tercer parametro, numero de trenes, debe ser mayor que cero y menor que 100.\n");
            terminarPrograma(1);
            exit(1);
        }
    }

    //////////////////////////////////////////////////////////////////
    ///             	INICIO DE PANTALLA PRINCIPAL	
    //////////////////////////////////////////////////////////////////
    if (LOMO_inicio(ret, tamMaximo, "i0546315", "i0939352") == -1) {
        fprintf(stderr, "Error al llamar LOMO_inicio\n");
        pon_error("ERROR, Problema al llamar LOMO_incio\n");
        terminarPrograma(1);
        return 100;
    }

    //////////////////////////////////////////////////////////////////
    ///             	MEDIDAS DE COMUNICACION 	
    //////////////////////////////////////////////////////////////////
    sem_EsperarPadre = CreateSemaphore(NULL, 0, nTrenes, NULL);
    if (sem_EsperarPadre == NULL) {
        fprintf(stderr, "Error al crear Semaforo EsperarPadre\n");
        terminarPrograma(1);
        return 100;
    }

    for (int s = 0; s < 392; s++) {
        semaforos[s] = CreateSemaphore(NULL, 1, 1, NULL);
        if (semaforos[s] == NULL) {
            fprintf(stderr, "Error al crear Semaforos\n");
            terminarPrograma(1);
            return 100;
        }
    }

    //rellenamos un mapa de 0
    int d, f;
    for (d = 0; d < 17; d++) {
        for (f = 0; f < 75; f++) {
            mapa[d][f] = 0;
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    //					CREACION DE TRENES
    ///////////////////////////////////////////////////////////////////////////
    for (i = 0; i < nTrenes; i++) {
        vectorTrenes[i] = CreateThread(NULL, 0, funcionTrenesHijos, LPVOID(i), 0, NULL);
        if (vectorTrenes[i] == NULL) {
            fprintf(stderr, "Error al crear trenes\n");
            terminarPrograma(1);
            return 100;
        }
    }


    //padre coge tantos recursos como trenes
    for (i = 0; i < nTrenes; i++) {
        if (WaitForSingleObject(sem_EsperarPadre, INFINITE) != WAIT_OBJECT_0) {
            pon_error("Error wait esperarPadre\n");
            terminarPrograma(1);
            return 100;
        }
    }

    terminarPrograma(1);
    return 0;
}

//////////////////////////////////////////////////////////////////
///             	FUNCIONES	
//////////////////////////////////////////////////////////////////
DWORD WINAPI funcionTrenesHijos(LPVOID param) {
    int coordY = 0;
    int xcab = 0, ycab = 0;
    int ycola = 0, xcola = 0;
    int i = (int)param;

    const int tiempoMax = 60000; //1 min en miliseg 60000
    DWORD tiempoComienzo = GetTickCount(); //nos da el tiempo inicial

    /*######################## PETICION DE IDENTIFICADOR ######################*/
    vectorIdTrenes[i] = LOMO_trenNuevo();
    if (vectorIdTrenes[i] == -1) {
        pon_error("Error en al asigna id LOMO_trenNuevo\n");
        terminarPrograma(1);
        return 100;
    }

    while (1) {

        ///////////////////////////////////////////////////////////////////
        ///     		TEMPORIZADOR DE UN MINUTO	
        ///////////////////////////////////////////////////////////////////
        if ((GetTickCount() - tiempoComienzo) >= tiempoMax) {
            pon_error("SE TERMINO EL TIEMPO DE EJECUCION MAXIMA");
            terminarPrograma(1);
            return 100;
        }

        /*######################## PETICION DE AVANCE ######################*/
        if ((LOMO_peticiOnAvance(vectorIdTrenes[i], &xcab, &ycab)) == -1) {
            pon_error("Error en la peticionAvance\n");
            terminarPrograma(1);
            return 100;
        }


        /*########### WAIT CRUCES #########################*/
        if (xcab == 16 && ycab == 0) {
            WaitForSingleObject(semaforos[363], INFINITE);
        }
        if (xcab == 36 && ycab == 0) {
            WaitForSingleObject(semaforos[364], INFINITE);
        }
        if (xcab == 54 && ycab == 0) {
            WaitForSingleObject(semaforos[365], INFINITE);
        }
        if (xcab == 68 && ycab == 0) {
            WaitForSingleObject(semaforos[366], INFINITE);
        }
        if (xcab == 74 && ycab == 0) {
            WaitForSingleObject(semaforos[367], INFINITE);
        }
        if (xcab == 16 && ycab == 4) {
            WaitForSingleObject(semaforos[368], INFINITE);
        }
        if (xcab == 16 && ycab == 7) {
            WaitForSingleObject(semaforos[369], INFINITE);
        }
        if (xcab == 36 && ycab == 7) {
            WaitForSingleObject(semaforos[370], INFINITE);
        }
        if (xcab == 54 && ycab == 7) {
            WaitForSingleObject(semaforos[371], INFINITE);
        }
        if (xcab == 68 && ycab == 7) {
            WaitForSingleObject(semaforos[372], INFINITE);
        }
        if (xcab == 74 && ycab == 7) {
            WaitForSingleObject(semaforos[373], INFINITE);
        }
        if (xcab == 0 && ycab == 9) {
            WaitForSingleObject(semaforos[374], INFINITE);
        }
        if (xcab == 16 && ycab == 9) {
            WaitForSingleObject(semaforos[375], INFINITE);
        }
        if (xcab == 0 && ycab == 12) {
            WaitForSingleObject(semaforos[376], INFINITE);
        }
        if (xcab == 16 && ycab == 12) {
            WaitForSingleObject(semaforos[377], INFINITE);
        }
        if (xcab == 36 && ycab == 12) {
            WaitForSingleObject(semaforos[378], INFINITE);
        }
        if (xcab == 54 && ycab == 12) {
            WaitForSingleObject(semaforos[379], INFINITE);
        }
        if (xcab == 68 && ycab == 12) {
            WaitForSingleObject(semaforos[380], INFINITE);
        }
        if (xcab == 74 && ycab == 12) {
            WaitForSingleObject(semaforos[381], INFINITE);
        }
        if (xcab == 0 && ycab == 16) {
            WaitForSingleObject(semaforos[382], INFINITE);
        }
        if (xcab == 16 && ycab == 16) {
            WaitForSingleObject(semaforos[383], INFINITE);
        }
        if (xcab == 36 && ycab == 16) {
            WaitForSingleObject(semaforos[384], INFINITE);
        }
        if (xcab == 54 && ycab == 16) {
            WaitForSingleObject(semaforos[385], INFINITE);
        }
        if (xcab == 68 && ycab == 16) {
            WaitForSingleObject(semaforos[386], INFINITE);
        }
        if (xcab == 74 && ycab == 16) {
            WaitForSingleObject(semaforos[387], INFINITE);
        }

        //primer trayecto
        if (xcab == 0 && ycab == 0) {
            WaitForSingleObject(semaforos[360], INFINITE); //sem1
        }
        if (xcab == 1 && ycab == 0) {
            WaitForSingleObject(semaforos[362], INFINITE); //sem3
        }
        if (xcab == 2 && ycab == 0) {
            WaitForSingleObject(semaforos[0], INFINITE);
        }
        if (xcab == 3 && ycab == 0) {
            WaitForSingleObject(semaforos[1], INFINITE);
        }
        if (xcab == 4 && ycab == 0) {
            WaitForSingleObject(semaforos[2], INFINITE);
        }
        if (xcab == 5 && ycab == 0) {
            WaitForSingleObject(semaforos[3], INFINITE);
        }
        if (xcab == 6 && ycab == 0) {
            WaitForSingleObject(semaforos[4], INFINITE);
        }
        if (xcab == 7 && ycab == 0) {
            WaitForSingleObject(semaforos[5], INFINITE);
        }
        if (xcab == 8 && ycab == 0) {
            WaitForSingleObject(semaforos[6], INFINITE);
        }
        if (xcab == 9 && ycab == 0) {
            WaitForSingleObject(semaforos[7], INFINITE);
        }
        if (xcab == 10 && ycab == 0) {
            WaitForSingleObject(semaforos[8], INFINITE);
        }
        if (xcab == 11 && ycab == 0) {
            WaitForSingleObject(semaforos[9], INFINITE);
        }
        if (xcab == 12 && ycab == 0) {
            WaitForSingleObject(semaforos[10], INFINITE);
        }
        if (xcab == 13 && ycab == 0) {
            WaitForSingleObject(semaforos[11], INFINITE);
        }
        if (xcab == 14 && ycab == 0) {
            WaitForSingleObject(semaforos[12], INFINITE);
        }
        if (xcab == 15 && ycab == 0) {
            WaitForSingleObject(semaforos[13], INFINITE);
        }
        if (xcab == 17 && ycab == 0) {
            WaitForSingleObject(semaforos[14], INFINITE);
        }
        if (xcab == 18 && ycab == 0) {
            WaitForSingleObject(semaforos[15], INFINITE);
        }
        if (xcab == 19 && ycab == 0) {
            WaitForSingleObject(semaforos[16], INFINITE);
        }
        if (xcab == 20 && ycab == 0) {
            WaitForSingleObject(semaforos[17], INFINITE);
        }
        if (xcab == 21 && ycab == 0) {
            WaitForSingleObject(semaforos[18], INFINITE);
        }
        if (xcab == 22 && ycab == 0) {
            WaitForSingleObject(semaforos[19], INFINITE);
        }
        if (xcab == 23 && ycab == 0) {
            WaitForSingleObject(semaforos[20], INFINITE);
        }
        if (xcab == 24 && ycab == 0) {
            WaitForSingleObject(semaforos[21], INFINITE);
        }
        if (xcab == 25 && ycab == 0) {
            WaitForSingleObject(semaforos[22], INFINITE);
        }
        if (xcab == 26 && ycab == 0) {
            WaitForSingleObject(semaforos[23], INFINITE);
        }
        if (xcab == 27 && ycab == 0) {
            WaitForSingleObject(semaforos[24], INFINITE);
        }
        if (xcab == 28 && ycab == 0) {
            WaitForSingleObject(semaforos[25], INFINITE);
        }
        if (xcab == 29 && ycab == 0) {
            WaitForSingleObject(semaforos[26], INFINITE);
        }
        if (xcab == 30 && ycab == 0) {
            WaitForSingleObject(semaforos[27], INFINITE);
        }
        if (xcab == 31 && ycab == 0) {
            WaitForSingleObject(semaforos[28], INFINITE);
        }
        if (xcab == 32 && ycab == 0) {
            WaitForSingleObject(semaforos[29], INFINITE);
        }
        if (xcab == 33 && ycab == 0) {
            WaitForSingleObject(semaforos[30], INFINITE);
        }
        if (xcab == 34 && ycab == 0) {
            WaitForSingleObject(semaforos[31], INFINITE);
        }
        if (xcab == 35 && ycab == 0) {
            WaitForSingleObject(semaforos[32], INFINITE);
        }
        if (xcab == 37 && ycab == 0) {
            WaitForSingleObject(semaforos[33], INFINITE);
        }
        if (xcab == 38 && ycab == 0) {
            WaitForSingleObject(semaforos[34], INFINITE);
        }
        if (xcab == 39 && ycab == 0) {
            WaitForSingleObject(semaforos[35], INFINITE);
        }
        if (xcab == 40 && ycab == 0) {
            WaitForSingleObject(semaforos[36], INFINITE);
        }
        if (xcab == 41 && ycab == 0) {
            WaitForSingleObject(semaforos[37], INFINITE);
        }
        if (xcab == 42 && ycab == 0) {
            WaitForSingleObject(semaforos[38], INFINITE);
        }
        if (xcab == 43 && ycab == 0) {
            WaitForSingleObject(semaforos[39], INFINITE);
        }
        if (xcab == 44 && ycab == 0) {
            WaitForSingleObject(semaforos[40], INFINITE);
        }
        if (xcab == 45 && ycab == 0) {
            WaitForSingleObject(semaforos[41], INFINITE);
        }
        if (xcab == 46 && ycab == 0) {
            WaitForSingleObject(semaforos[42], INFINITE);
        }
        if (xcab == 47 && ycab == 0) {
            WaitForSingleObject(semaforos[43], INFINITE);
        }
        if (xcab == 48 && ycab == 0) {
            WaitForSingleObject(semaforos[44], INFINITE);
        }
        if (xcab == 49 && ycab == 0) {
            WaitForSingleObject(semaforos[45], INFINITE);
        }
        if (xcab == 50 && ycab == 0) {
            WaitForSingleObject(semaforos[46], INFINITE);
        }
        if (xcab == 51 && ycab == 0) {
            WaitForSingleObject(semaforos[47], INFINITE);
        }
        if (xcab == 52 && ycab == 0) {
            WaitForSingleObject(semaforos[48], INFINITE);
        }
        if (xcab == 53 && ycab == 0) {
            WaitForSingleObject(semaforos[49], INFINITE);
        }
        if (xcab == 55 && ycab == 0) {
            WaitForSingleObject(semaforos[50], INFINITE);
        }
        if (xcab == 56 && ycab == 0) {
            WaitForSingleObject(semaforos[51], INFINITE);
        }
        if (xcab == 57 && ycab == 0) {
            WaitForSingleObject(semaforos[52], INFINITE);
        }
        if (xcab == 58 && ycab == 0) {
            WaitForSingleObject(semaforos[53], INFINITE);
        }
        if (xcab == 59 && ycab == 0) {
            WaitForSingleObject(semaforos[54], INFINITE);
        }
        if (xcab == 60 && ycab == 0) {
            WaitForSingleObject(semaforos[55], INFINITE);
        }
        if (xcab == 61 && ycab == 0) {
            WaitForSingleObject(semaforos[56], INFINITE);
        }
        if (xcab == 62 && ycab == 0) {
            WaitForSingleObject(semaforos[57], INFINITE);
        }
        if (xcab == 63 && ycab == 0) {
            WaitForSingleObject(semaforos[58], INFINITE);
        }
        if (xcab == 64 && ycab == 0) {
            WaitForSingleObject(semaforos[59], INFINITE);
        }
        if (xcab == 65 && ycab == 0) {
            WaitForSingleObject(semaforos[60], INFINITE);
        }
        if (xcab == 66 && ycab == 0) {
            WaitForSingleObject(semaforos[61], INFINITE);
        }
        if (xcab == 67 && ycab == 0) {
            WaitForSingleObject(semaforos[62], INFINITE);
        }
        if (xcab == 69 && ycab == 0) {
            WaitForSingleObject(semaforos[63], INFINITE);
        }
        if (xcab == 70 && ycab == 0) {
            WaitForSingleObject(semaforos[64], INFINITE);
        }
        if (xcab == 71 && ycab == 0) {
            WaitForSingleObject(semaforos[65], INFINITE);
        }
        if (xcab == 72 && ycab == 0) {
            WaitForSingleObject(semaforos[66], INFINITE);
        }
        if (xcab == 73 && ycab == 0) {
            WaitForSingleObject(semaforos[67], INFINITE);
        }

        //segunda linea horizontal
        if (xcab == 0 && ycab == 4) {
            WaitForSingleObject(semaforos[68], INFINITE);
        }
        if (xcab == 1 && ycab == 4) {
            WaitForSingleObject(semaforos[69], INFINITE);
        }
        if (xcab == 2 && ycab == 4) {
            WaitForSingleObject(semaforos[70], INFINITE);
        }
        if (xcab == 3 && ycab == 4) {
            WaitForSingleObject(semaforos[71], INFINITE);
        }
        if (xcab == 4 && ycab == 4) {
            WaitForSingleObject(semaforos[72], INFINITE);
        }
        if (xcab == 5 && ycab == 4) {
            WaitForSingleObject(semaforos[73], INFINITE);
        }
        if (xcab == 6 && ycab == 4) {
            WaitForSingleObject(semaforos[74], INFINITE);
        }
        if (xcab == 7 && ycab == 4) {
            WaitForSingleObject(semaforos[75], INFINITE);
        }
        if (xcab == 8 && ycab == 4) {
            WaitForSingleObject(semaforos[76], INFINITE);
        }
        if (xcab == 9 && ycab == 4) {
            WaitForSingleObject(semaforos[77], INFINITE);
        }
        if (xcab == 10 && ycab == 4) {
            WaitForSingleObject(semaforos[78], INFINITE);
        }
        if (xcab == 11 && ycab == 4) {
            WaitForSingleObject(semaforos[79], INFINITE);
        }
        if (xcab == 12 && ycab == 4) {
            WaitForSingleObject(semaforos[80], INFINITE);
        }
        if (xcab == 13 && ycab == 4) {
            WaitForSingleObject(semaforos[81], INFINITE);
        }
        if (xcab == 14 && ycab == 4) {
            WaitForSingleObject(semaforos[82], INFINITE);
        }
        if (xcab == 15 && ycab == 4) {
            WaitForSingleObject(semaforos[83], INFINITE);
        }
        //tercera trayectoria horizontal
        if (xcab == 17 && ycab == 7) {
            WaitForSingleObject(semaforos[84], INFINITE);
        }
        if (xcab == 18 && ycab == 7) {
            WaitForSingleObject(semaforos[85], INFINITE);
        }
        if (xcab == 19 && ycab == 7) {
            WaitForSingleObject(semaforos[86], INFINITE);
        }
        if (xcab == 20 && ycab == 7) {
            WaitForSingleObject(semaforos[87], INFINITE);
        }
        if (xcab == 21 && ycab == 7) {
            WaitForSingleObject(semaforos[88], INFINITE);
        }
        if (xcab == 22 && ycab == 7) {
            WaitForSingleObject(semaforos[89], INFINITE);
        }
        if (xcab == 23 && ycab == 7) {
            WaitForSingleObject(semaforos[90], INFINITE);
        }
        if (xcab == 24 && ycab == 7) {
            WaitForSingleObject(semaforos[91], INFINITE);
        }
        if (xcab == 25 && ycab == 7) {
            WaitForSingleObject(semaforos[92], INFINITE);
        }
        if (xcab == 26 && ycab == 7) {
            WaitForSingleObject(semaforos[93], INFINITE);
        }
        if (xcab == 27 && ycab == 7) {
            WaitForSingleObject(semaforos[94], INFINITE);
        }
        if (xcab == 28 && ycab == 7) {
            WaitForSingleObject(semaforos[95], INFINITE);
        }
        if (xcab == 29 && ycab == 7) {
            WaitForSingleObject(semaforos[96], INFINITE);
        }
        if (xcab == 30 && ycab == 7) {
            WaitForSingleObject(semaforos[97], INFINITE);
        }
        if (xcab == 31 && ycab == 7) {
            WaitForSingleObject(semaforos[98], INFINITE);
        }
        if (xcab == 32 && ycab == 7) {
            WaitForSingleObject(semaforos[99], INFINITE);
        }
        if (xcab == 33 && ycab == 7) {
            WaitForSingleObject(semaforos[100], INFINITE);
        }
        if (xcab == 34 && ycab == 7) {
            WaitForSingleObject(semaforos[101], INFINITE);
        }
        if (xcab == 35 && ycab == 7) {
            WaitForSingleObject(semaforos[102], INFINITE);
        }
        if (xcab == 37 && ycab == 7) {
            WaitForSingleObject(semaforos[103], INFINITE);
        }
        if (xcab == 38 && ycab == 7) {
            WaitForSingleObject(semaforos[104], INFINITE);
        }
        if (xcab == 39 && ycab == 7) {
            WaitForSingleObject(semaforos[105], INFINITE);
        }
        if (xcab == 40 && ycab == 7) {
            WaitForSingleObject(semaforos[106], INFINITE);
        }
        if (xcab == 41 && ycab == 7) {
            WaitForSingleObject(semaforos[107], INFINITE);
        }
        if (xcab == 42 && ycab == 7) {
            WaitForSingleObject(semaforos[108], INFINITE);
        }
        if (xcab == 43 && ycab == 7) {
            WaitForSingleObject(semaforos[109], INFINITE);
        }
        if (xcab == 44 && ycab == 7) {
            WaitForSingleObject(semaforos[110], INFINITE);
        }
        if (xcab == 45 && ycab == 7) {
            WaitForSingleObject(semaforos[111], INFINITE);
        }
        if (xcab == 46 && ycab == 7) {
            WaitForSingleObject(semaforos[112], INFINITE);
        }
        if (xcab == 47 && ycab == 7) {
            WaitForSingleObject(semaforos[113], INFINITE);
        }
        if (xcab == 48 && ycab == 7) {
            WaitForSingleObject(semaforos[114], INFINITE);
        }
        if (xcab == 49 && ycab == 7) {
            WaitForSingleObject(semaforos[115], INFINITE);
        }
        if (xcab == 50 && ycab == 7) {
            WaitForSingleObject(semaforos[116], INFINITE);
        }
        if (xcab == 51 && ycab == 7) {
            WaitForSingleObject(semaforos[117], INFINITE);
        }
        if (xcab == 52 && ycab == 7) {
            WaitForSingleObject(semaforos[118], INFINITE);
        }
        if (xcab == 53 && ycab == 7) {
            WaitForSingleObject(semaforos[119], INFINITE);
        }
        if (xcab == 55 && ycab == 7) {
            WaitForSingleObject(semaforos[120], INFINITE);
        }
        if (xcab == 56 && ycab == 7) {
            WaitForSingleObject(semaforos[121], INFINITE);
        }
        if (xcab == 57 && ycab == 7) {
            WaitForSingleObject(semaforos[122], INFINITE);
        }
        if (xcab == 58 && ycab == 7) {
            WaitForSingleObject(semaforos[123], INFINITE);
        }
        if (xcab == 59 && ycab == 7) {
            WaitForSingleObject(semaforos[124], INFINITE);
        }
        if (xcab == 60 && ycab == 7) {
            WaitForSingleObject(semaforos[125], INFINITE);
        }
        if (xcab == 61 && ycab == 7) {
            WaitForSingleObject(semaforos[126], INFINITE);
        }
        if (xcab == 62 && ycab == 7) {
            WaitForSingleObject(semaforos[127], INFINITE);
        }
        if (xcab == 63 && ycab == 7) {
            WaitForSingleObject(semaforos[128], INFINITE);
        }
        if (xcab == 64 && ycab == 7) {
            WaitForSingleObject(semaforos[129], INFINITE);
        }
        if (xcab == 65 && ycab == 7) {
            WaitForSingleObject(semaforos[130], INFINITE);
        }
        if (xcab == 66 && ycab == 7) {
            WaitForSingleObject(semaforos[131], INFINITE);
        }
        if (xcab == 67 && ycab == 7) {
            WaitForSingleObject(semaforos[132], INFINITE);
        }
        if (xcab == 69 && ycab == 7) {
            WaitForSingleObject(semaforos[133], INFINITE);
        }
        if (xcab == 70 && ycab == 7) {
            WaitForSingleObject(semaforos[134], INFINITE);
        }
        if (xcab == 71 && ycab == 7) {
            WaitForSingleObject(semaforos[135], INFINITE);
        }
        if (xcab == 72 && ycab == 7) {
            WaitForSingleObject(semaforos[136], INFINITE);
        }
        if (xcab == 73 && ycab == 7) {
            WaitForSingleObject(semaforos[137], INFINITE);
        }
        //cuarta trayectoria horizontal
        if (xcab == 1 && ycab == 9) {
            WaitForSingleObject(semaforos[138], INFINITE);
        }
        if (xcab == 2 && ycab == 9) {
            WaitForSingleObject(semaforos[139], INFINITE);
        }
        if (xcab == 3 && ycab == 9) {
            WaitForSingleObject(semaforos[140], INFINITE);
        }
        if (xcab == 4 && ycab == 9) {
            WaitForSingleObject(semaforos[141], INFINITE);
        }
        if (xcab == 5 && ycab == 9) {
            WaitForSingleObject(semaforos[142], INFINITE);
        }
        if (xcab == 6 && ycab == 9) {
            WaitForSingleObject(semaforos[143], INFINITE);
        }
        if (xcab == 7 && ycab == 9) {
            WaitForSingleObject(semaforos[144], INFINITE);
        }
        if (xcab == 8 && ycab == 9) {
            WaitForSingleObject(semaforos[145], INFINITE);
        }
        if (xcab == 9 && ycab == 9) {
            WaitForSingleObject(semaforos[146], INFINITE);
        }
        if (xcab == 10 && ycab == 9) {
            WaitForSingleObject(semaforos[147], INFINITE);
        }
        if (xcab == 11 && ycab == 9) {
            WaitForSingleObject(semaforos[148], INFINITE);
        }
        if (xcab == 12 && ycab == 9) {
            WaitForSingleObject(semaforos[149], INFINITE);
        }
        if (xcab == 13 && ycab == 9) {
            WaitForSingleObject(semaforos[150], INFINITE);
        }
        if (xcab == 14 && ycab == 9) {
            WaitForSingleObject(semaforos[151], INFINITE);
        }
        if (xcab == 15 && ycab == 9) {
            WaitForSingleObject(semaforos[152], INFINITE);
        }
        //quinta trayectoria horizontal
        if (xcab == 1 && ycab == 12) {
            WaitForSingleObject(semaforos[153], INFINITE);
        }
        if (xcab == 2 && ycab == 12) {
            WaitForSingleObject(semaforos[154], INFINITE);
        }
        if (xcab == 3 && ycab == 12) {
            WaitForSingleObject(semaforos[155], INFINITE);
        }
        if (xcab == 4 && ycab == 12) {
            WaitForSingleObject(semaforos[156], INFINITE);
        }
        if (xcab == 5 && ycab == 12) {
            WaitForSingleObject(semaforos[157], INFINITE);
        }
        if (xcab == 6 && ycab == 12) {
            WaitForSingleObject(semaforos[158], INFINITE);
        }
        if (xcab == 7 && ycab == 12) {
            WaitForSingleObject(semaforos[159], INFINITE);
        }
        if (xcab == 8 && ycab == 12) {
            WaitForSingleObject(semaforos[160], INFINITE);
        }
        if (xcab == 9 && ycab == 12) {
            WaitForSingleObject(semaforos[161], INFINITE);
        }
        if (xcab == 10 && ycab == 12) {
            WaitForSingleObject(semaforos[162], INFINITE);
        }
        if (xcab == 11 && ycab == 12) {
            WaitForSingleObject(semaforos[163], INFINITE);
        }
        if (xcab == 12 && ycab == 12) {
            WaitForSingleObject(semaforos[164], INFINITE);
        }
        if (xcab == 13 && ycab == 12) {
            WaitForSingleObject(semaforos[165], INFINITE);
        }
        if (xcab == 14 && ycab == 12) {
            WaitForSingleObject(semaforos[166], INFINITE);
        }
        if (xcab == 15 && ycab == 12) {
            WaitForSingleObject(semaforos[167], INFINITE);
        }
        if (xcab == 17 && ycab == 12) {
            WaitForSingleObject(semaforos[168], INFINITE);
        }
        if (xcab == 18 && ycab == 12) {
            WaitForSingleObject(semaforos[169], INFINITE);
        }
        if (xcab == 19 && ycab == 12) {
            WaitForSingleObject(semaforos[170], INFINITE);
        }
        if (xcab == 20 && ycab == 12) {
            WaitForSingleObject(semaforos[171], INFINITE);
        }
        if (xcab == 21 && ycab == 12) {
            WaitForSingleObject(semaforos[172], INFINITE);
        }
        if (xcab == 22 && ycab == 12) {
            WaitForSingleObject(semaforos[173], INFINITE);
        }
        if (xcab == 23 && ycab == 12) {
            WaitForSingleObject(semaforos[174], INFINITE);
        }
        if (xcab == 24 && ycab == 12) {
            WaitForSingleObject(semaforos[175], INFINITE);
        }
        if (xcab == 25 && ycab == 12) {
            WaitForSingleObject(semaforos[176], INFINITE);
        }
        if (xcab == 26 && ycab == 12) {
            WaitForSingleObject(semaforos[177], INFINITE);
        }
        if (xcab == 27 && ycab == 12) {
            WaitForSingleObject(semaforos[178], INFINITE);
        }
        if (xcab == 28 && ycab == 12) {
            WaitForSingleObject(semaforos[179], INFINITE);
        }
        if (xcab == 29 && ycab == 12) {
            WaitForSingleObject(semaforos[180], INFINITE);
        }
        if (xcab == 30 && ycab == 12) {
            WaitForSingleObject(semaforos[181], INFINITE);
        }
        if (xcab == 31 && ycab == 12) {
            WaitForSingleObject(semaforos[182], INFINITE);
        }
        if (xcab == 32 && ycab == 12) {
            WaitForSingleObject(semaforos[183], INFINITE);
        }
        if (xcab == 33 && ycab == 12) {
            WaitForSingleObject(semaforos[184], INFINITE);
        }
        if (xcab == 34 && ycab == 12) {
            WaitForSingleObject(semaforos[185], INFINITE);
        }
        if (xcab == 35 && ycab == 12) {
            WaitForSingleObject(semaforos[186], INFINITE);
        }
        if (xcab == 37 && ycab == 12) {
            WaitForSingleObject(semaforos[187], INFINITE);
        }
        if (xcab == 38 && ycab == 12) {
            WaitForSingleObject(semaforos[188], INFINITE);
        }
        if (xcab == 39 && ycab == 12) {
            WaitForSingleObject(semaforos[189], INFINITE);
        }
        if (xcab == 40 && ycab == 12) {
            WaitForSingleObject(semaforos[190], INFINITE);
        }
        if (xcab == 41 && ycab == 12) {
            WaitForSingleObject(semaforos[191], INFINITE);
        }
        if (xcab == 42 && ycab == 12) {
            WaitForSingleObject(semaforos[192], INFINITE);
        }
        if (xcab == 43 && ycab == 12) {
            WaitForSingleObject(semaforos[193], INFINITE);
        }
        if (xcab == 44 && ycab == 12) {
            WaitForSingleObject(semaforos[194], INFINITE);
        }
        if (xcab == 45 && ycab == 12) {
            WaitForSingleObject(semaforos[195], INFINITE);
        }
        if (xcab == 46 && ycab == 12) {
            WaitForSingleObject(semaforos[196], INFINITE);
        }
        if (xcab == 47 && ycab == 12) {
            WaitForSingleObject(semaforos[197], INFINITE);
        }
        if (xcab == 48 && ycab == 12) {
            WaitForSingleObject(semaforos[198], INFINITE);
        }
        if (xcab == 49 && ycab == 12) {
            WaitForSingleObject(semaforos[199], INFINITE);
        }
        if (xcab == 50 && ycab == 12) {
            WaitForSingleObject(semaforos[200], INFINITE);
        }
        if (xcab == 51 && ycab == 12) {
            WaitForSingleObject(semaforos[201], INFINITE);
        }
        if (xcab == 52 && ycab == 12) {
            WaitForSingleObject(semaforos[202], INFINITE);
        }
        if (xcab == 53 && ycab == 12) {
            WaitForSingleObject(semaforos[203], INFINITE);
        }
        if (xcab == 55 && ycab == 12) {
            WaitForSingleObject(semaforos[204], INFINITE);
        }
        if (xcab == 56 && ycab == 12) {
            WaitForSingleObject(semaforos[205], INFINITE);
        }
        if (xcab == 57 && ycab == 12) {
            WaitForSingleObject(semaforos[206], INFINITE);
        }
        if (xcab == 58 && ycab == 12) {
            WaitForSingleObject(semaforos[207], INFINITE);
        }
        if (xcab == 59 && ycab == 12) {
            WaitForSingleObject(semaforos[208], INFINITE);
        }
        if (xcab == 60 && ycab == 12) {
            WaitForSingleObject(semaforos[209], INFINITE);
        }
        if (xcab == 61 && ycab == 12) {
            WaitForSingleObject(semaforos[210], INFINITE);
        }
        if (xcab == 62 && ycab == 12) {
            WaitForSingleObject(semaforos[211], INFINITE);
        }
        if (xcab == 63 && ycab == 12) {
            WaitForSingleObject(semaforos[212], INFINITE);
        }
        if (xcab == 64 && ycab == 12) {
            WaitForSingleObject(semaforos[213], INFINITE);
        }
        if (xcab == 65 && ycab == 12) {
            WaitForSingleObject(semaforos[214], INFINITE);
        }
        if (xcab == 66 && ycab == 12) {
            WaitForSingleObject(semaforos[215], INFINITE);
        }
        if (xcab == 67 && ycab == 12) {
            WaitForSingleObject(semaforos[216], INFINITE);
        }
        if (xcab == 69 && ycab == 12) {
            WaitForSingleObject(semaforos[217], INFINITE);
        }
        if (xcab == 70 && ycab == 12) {
            WaitForSingleObject(semaforos[218], INFINITE);
        }
        if (xcab == 71 && ycab == 12) {
            WaitForSingleObject(semaforos[219], INFINITE);
        }
        if (xcab == 72 && ycab == 12) {
            WaitForSingleObject(semaforos[220], INFINITE);
        }
        if (xcab == 73 && ycab == 12) {
            WaitForSingleObject(semaforos[221], INFINITE);
        }
        //sexta trayectoria horizontal
        if (xcab == 1 && ycab == 16) {
            WaitForSingleObject(semaforos[222], INFINITE);
        }
        if (xcab == 2 && ycab == 16) {
            WaitForSingleObject(semaforos[223], INFINITE);
        }
        if (xcab == 3 && ycab == 16) {
            WaitForSingleObject(semaforos[224], INFINITE);
        }
        if (xcab == 4 && ycab == 16) {
            WaitForSingleObject(semaforos[225], INFINITE);
        }
        if (xcab == 5 && ycab == 16) {
            WaitForSingleObject(semaforos[226], INFINITE);
        }
        if (xcab == 6 && ycab == 16) {
            WaitForSingleObject(semaforos[227], INFINITE);
        }
        if (xcab == 7 && ycab == 16) {
            WaitForSingleObject(semaforos[228], INFINITE);
        }
        if (xcab == 8 && ycab == 16) {
            WaitForSingleObject(semaforos[229], INFINITE);
        }
        if (xcab == 9 && ycab == 16) {
            WaitForSingleObject(semaforos[230], INFINITE);
        }
        if (xcab == 10 && ycab == 16) {
            WaitForSingleObject(semaforos[231], INFINITE);
        }
        if (xcab == 11 && ycab == 16) {
            WaitForSingleObject(semaforos[232], INFINITE);
        }
        if (xcab == 12 && ycab == 16) {
            WaitForSingleObject(semaforos[233], INFINITE);
        }
        if (xcab == 13 && ycab == 16) {
            WaitForSingleObject(semaforos[234], INFINITE);
        }
        if (xcab == 14 && ycab == 16) {
            WaitForSingleObject(semaforos[235], INFINITE);
        }
        if (xcab == 15 && ycab == 16) {
            WaitForSingleObject(semaforos[236], INFINITE);
        }
        if (xcab == 17 && ycab == 16) {
            WaitForSingleObject(semaforos[237], INFINITE);
        }
        if (xcab == 18 && ycab == 16) {
            WaitForSingleObject(semaforos[238], INFINITE);
        }
        if (xcab == 19 && ycab == 16) {
            WaitForSingleObject(semaforos[239], INFINITE);
        }
        if (xcab == 20 && ycab == 16) {
            WaitForSingleObject(semaforos[240], INFINITE);
        }
        if (xcab == 21 && ycab == 16) {
            WaitForSingleObject(semaforos[241], INFINITE);
        }
        if (xcab == 22 && ycab == 16) {
            WaitForSingleObject(semaforos[242], INFINITE);
        }
        if (xcab == 23 && ycab == 16) {
            WaitForSingleObject(semaforos[243], INFINITE);
        }
        if (xcab == 24 && ycab == 16) {
            WaitForSingleObject(semaforos[244], INFINITE);
        }
        if (xcab == 25 && ycab == 16) {
            WaitForSingleObject(semaforos[245], INFINITE);
        }
        if (xcab == 26 && ycab == 16) {
            WaitForSingleObject(semaforos[246], INFINITE);
        }
        if (xcab == 27 && ycab == 16) {
            WaitForSingleObject(semaforos[247], INFINITE);
        }
        if (xcab == 28 && ycab == 16) {
            WaitForSingleObject(semaforos[248], INFINITE);
        }
        if (xcab == 29 && ycab == 16) {
            WaitForSingleObject(semaforos[249], INFINITE);
        }
        if (xcab == 30 && ycab == 16) {
            WaitForSingleObject(semaforos[250], INFINITE);
        }
        if (xcab == 31 && ycab == 16) {
            WaitForSingleObject(semaforos[251], INFINITE);
        }
        if (xcab == 32 && ycab == 16) {
            WaitForSingleObject(semaforos[252], INFINITE);
        }
        if (xcab == 33 && ycab == 16) {
            WaitForSingleObject(semaforos[253], INFINITE);
        }
        if (xcab == 34 && ycab == 16) {
            WaitForSingleObject(semaforos[254], INFINITE);
        }
        if (xcab == 35 && ycab == 16) {
            WaitForSingleObject(semaforos[255], INFINITE);
        }
        if (xcab == 37 && ycab == 16) {
            WaitForSingleObject(semaforos[256], INFINITE);
        }
        if (xcab == 38 && ycab == 16) {
            WaitForSingleObject(semaforos[257], INFINITE);
        }
        if (xcab == 39 && ycab == 16) {
            WaitForSingleObject(semaforos[258], INFINITE);
        }
        if (xcab == 40 && ycab == 16) {
            WaitForSingleObject(semaforos[259], INFINITE);
        }
        if (xcab == 41 && ycab == 16) {
            WaitForSingleObject(semaforos[260], INFINITE);
        }
        if (xcab == 42 && ycab == 16) {
            WaitForSingleObject(semaforos[261], INFINITE);
        }
        if (xcab == 43 && ycab == 16) {
            WaitForSingleObject(semaforos[262], INFINITE);
        }
        if (xcab == 44 && ycab == 16) {
            WaitForSingleObject(semaforos[263], INFINITE);
        }
        if (xcab == 45 && ycab == 16) {
            WaitForSingleObject(semaforos[264], INFINITE);
        }
        if (xcab == 46 && ycab == 16) {
            WaitForSingleObject(semaforos[265], INFINITE);
        }
        if (xcab == 47 && ycab == 16) {
            WaitForSingleObject(semaforos[266], INFINITE);
        }
        if (xcab == 48 && ycab == 16) {
            WaitForSingleObject(semaforos[267], INFINITE);
        }
        if (xcab == 49 && ycab == 16) {
            WaitForSingleObject(semaforos[268], INFINITE);
        }
        if (xcab == 50 && ycab == 16) {
            WaitForSingleObject(semaforos[269], INFINITE);
        }
        if (xcab == 51 && ycab == 16) {
            WaitForSingleObject(semaforos[270], INFINITE);
        }
        if (xcab == 52 && ycab == 16) {
            WaitForSingleObject(semaforos[271], INFINITE);
        }
        if (xcab == 53 && ycab == 16) {
            WaitForSingleObject(semaforos[272], INFINITE);
        }
        if (xcab == 55 && ycab == 16) {
            WaitForSingleObject(semaforos[273], INFINITE);
        }
        if (xcab == 56 && ycab == 16) {
            WaitForSingleObject(semaforos[274], INFINITE);
        }
        if (xcab == 57 && ycab == 16) {
            WaitForSingleObject(semaforos[275], INFINITE);
        }
        if (xcab == 58 && ycab == 16) {
            WaitForSingleObject(semaforos[276], INFINITE);
        }
        if (xcab == 59 && ycab == 16) {
            WaitForSingleObject(semaforos[277], INFINITE);
        }
        if (xcab == 60 && ycab == 16) {
            WaitForSingleObject(semaforos[278], INFINITE);
        }
        if (xcab == 61 && ycab == 16) {
            WaitForSingleObject(semaforos[279], INFINITE);
        }
        if (xcab == 62 && ycab == 16) {
            WaitForSingleObject(semaforos[280], INFINITE);
        }
        if (xcab == 63 && ycab == 16) {
            WaitForSingleObject(semaforos[281], INFINITE);
        }
        if (xcab == 64 && ycab == 16) {
            WaitForSingleObject(semaforos[282], INFINITE);
        }
        if (xcab == 65 && ycab == 16) {
            WaitForSingleObject(semaforos[283], INFINITE);
        }
        if (xcab == 66 && ycab == 16) {
            WaitForSingleObject(semaforos[284], INFINITE);
        }
        if (xcab == 67 && ycab == 16) {
            WaitForSingleObject(semaforos[285], INFINITE);
        }
        if (xcab == 69 && ycab == 16) {
            WaitForSingleObject(semaforos[286], INFINITE);
        }
        if (xcab == 70 && ycab == 16) {
            WaitForSingleObject(semaforos[287], INFINITE);
        }
        if (xcab == 71 && ycab == 16) {
            WaitForSingleObject(semaforos[288], INFINITE);
        }
        if (xcab == 72 && ycab == 16) {
            WaitForSingleObject(semaforos[289], INFINITE);
        }
        if (xcab == 73 && ycab == 16) {
            WaitForSingleObject(semaforos[290], INFINITE);
        }

        //primer trayectoria vertical
        if (xcab == 0 && ycab == 4) {
            WaitForSingleObject(semaforos[291], INFINITE);
        }
        if (xcab == 0 && ycab == 5) {
            WaitForSingleObject(semaforos[292], INFINITE);
        }
        if (xcab == 0 && ycab == 6) {
            WaitForSingleObject(semaforos[293], INFINITE);
        }
        if (xcab == 0 && ycab == 7) {
            WaitForSingleObject(semaforos[294], INFINITE);
        }
        if (xcab == 0 && ycab == 8) {
            WaitForSingleObject(semaforos[295], INFINITE);
        }
        if (xcab == 0 && ycab == 10) {
            WaitForSingleObject(semaforos[296], INFINITE);
        }
        if (xcab == 0 && ycab == 11) {
            WaitForSingleObject(semaforos[297], INFINITE);
        }
        if (xcab == 0 && ycab == 13) {
            WaitForSingleObject(semaforos[298], INFINITE);
        }
        if (xcab == 0 && ycab == 14) {
            WaitForSingleObject(semaforos[299], INFINITE);
        }
        if (xcab == 0 && ycab == 15) {
            WaitForSingleObject(semaforos[300], INFINITE);
        }
        //segunda trayectoria vertical
        if (xcab == 16 && ycab == 1) {
            WaitForSingleObject(semaforos[301], INFINITE);
        }
        if (xcab == 16 && ycab == 2) {
            WaitForSingleObject(semaforos[302], INFINITE);
        }
        if (xcab == 16 && ycab == 3) {
            WaitForSingleObject(semaforos[303], INFINITE);
        }
        if (xcab == 16 && ycab == 5) {
            WaitForSingleObject(semaforos[304], INFINITE);
        }
        if (xcab == 16 && ycab == 6) {
            WaitForSingleObject(semaforos[305], INFINITE);
        }
        if (xcab == 16 && ycab == 8) {
            WaitForSingleObject(semaforos[306], INFINITE);
        }
        if (xcab == 16 && ycab == 10) {
            WaitForSingleObject(semaforos[307], INFINITE);
        }
        if (xcab == 16 && ycab == 11) {
            WaitForSingleObject(semaforos[308], INFINITE);
        }
        if (xcab == 16 && ycab == 13) {
            WaitForSingleObject(semaforos[309], INFINITE);
        }
        if (xcab == 16 && ycab == 14) {
            WaitForSingleObject(semaforos[310], INFINITE);
        }
        if (xcab == 16 && ycab == 15) {
            WaitForSingleObject(semaforos[311], INFINITE);
        }
        //tercera trayectoria vertical
        if (xcab == 36 && ycab == 1) {
            WaitForSingleObject(semaforos[312], INFINITE);
        }
        if (xcab == 36 && ycab == 2) {
            WaitForSingleObject(semaforos[313], INFINITE);
        }
        if (xcab == 36 && ycab == 3) {
            WaitForSingleObject(semaforos[314], INFINITE);
        }
        if (xcab == 36 && ycab == 4) {
            WaitForSingleObject(semaforos[388], INFINITE);
        }
        if (xcab == 36 && ycab == 5) {
            WaitForSingleObject(semaforos[315], INFINITE);
        }
        if (xcab == 36 && ycab == 6) {
            WaitForSingleObject(semaforos[316], INFINITE);
        }
        if (xcab == 36 && ycab == 8) {
            WaitForSingleObject(semaforos[317], INFINITE);
        }
        if (xcab == 36 && ycab == 9) {
            WaitForSingleObject(semaforos[318], INFINITE);
        }
        if (xcab == 36 && ycab == 10) {
            WaitForSingleObject(semaforos[319], INFINITE);
        }
        if (xcab == 36 && ycab == 11) {
            WaitForSingleObject(semaforos[320], INFINITE);
        }
        if (xcab == 36 && ycab == 13) {
            WaitForSingleObject(semaforos[321], INFINITE);
        }
        if (xcab == 36 && ycab == 14) {
            WaitForSingleObject(semaforos[322], INFINITE);
        }
        if (xcab == 36 && ycab == 15) {
            WaitForSingleObject(semaforos[323], INFINITE);
        }
        //cuarta trayectoria
        if (xcab == 54 && ycab == 1) {
            WaitForSingleObject(semaforos[324], INFINITE);
        }
        if (xcab == 54 && ycab == 2) {
            WaitForSingleObject(semaforos[325], INFINITE);
        }
        if (xcab == 54 && ycab == 3) {
            WaitForSingleObject(semaforos[326], INFINITE);
        }
        if (xcab == 54 && ycab == 4) {
            WaitForSingleObject(semaforos[389], INFINITE);
        }
        if (xcab == 54 && ycab == 5) {
            WaitForSingleObject(semaforos[327], INFINITE);
        }
        if (xcab == 54 && ycab == 6) {
            WaitForSingleObject(semaforos[328], INFINITE);
        }
        if (xcab == 54 && ycab == 8) {
            WaitForSingleObject(semaforos[329], INFINITE);
        }
        if (xcab == 54 && ycab == 9) {
            WaitForSingleObject(semaforos[330], INFINITE);
        }
        if (xcab == 54 && ycab == 10) {
            WaitForSingleObject(semaforos[331], INFINITE);
        }
        if (xcab == 54 && ycab == 11) {
            WaitForSingleObject(semaforos[332], INFINITE);
        }
        if (xcab == 54 && ycab == 13) {
            WaitForSingleObject(semaforos[333], INFINITE);
        }
        if (xcab == 54 && ycab == 14) {
            WaitForSingleObject(semaforos[334], INFINITE);
        }
        if (xcab == 54 && ycab == 15) {
            WaitForSingleObject(semaforos[335], INFINITE);
        }
        //quinta trayectoria
        if (xcab == 68 && ycab == 1) {
            WaitForSingleObject(semaforos[336], INFINITE);
        }
        if (xcab == 68 && ycab == 2) {
            WaitForSingleObject(semaforos[337], INFINITE);
        }
        if (xcab == 68 && ycab == 3) {
            WaitForSingleObject(semaforos[338], INFINITE);
        }
        if (xcab == 68 && ycab == 4) {
            WaitForSingleObject(semaforos[390], INFINITE);
        }
        if (xcab == 68 && ycab == 5) {
            WaitForSingleObject(semaforos[339], INFINITE);
        }
        if (xcab == 68 && ycab == 6) {
            WaitForSingleObject(semaforos[340], INFINITE);
        }
        if (xcab == 68 && ycab == 8) {
            WaitForSingleObject(semaforos[341], INFINITE);
        }
        if (xcab == 68 && ycab == 9) {
            WaitForSingleObject(semaforos[342], INFINITE);
        }
        if (xcab == 68 && ycab == 10) {
            WaitForSingleObject(semaforos[343], INFINITE);
        }
        if (xcab == 68 && ycab == 11) {
            WaitForSingleObject(semaforos[344], INFINITE);
        }
        if (xcab == 68 && ycab == 13) {
            WaitForSingleObject(semaforos[345], INFINITE);
        }
        if (xcab == 68 && ycab == 14) {
            WaitForSingleObject(semaforos[346], INFINITE);
        }
        if (xcab == 68 && ycab == 15) {
            WaitForSingleObject(semaforos[347], INFINITE);
        }
        //ultima trayectoria vertical
        if (xcab == 74 && ycab == 1) {
            WaitForSingleObject(semaforos[348], INFINITE);
        }
        if (xcab == 74 && ycab == 2) {
            WaitForSingleObject(semaforos[349], INFINITE);
        }
        if (xcab == 74 && ycab == 3) {
            WaitForSingleObject(semaforos[350], INFINITE);
        }
        if (xcab == 74 && ycab == 4) {
            WaitForSingleObject(semaforos[391], INFINITE); /////////////////////////////////////////////////////////
        }
        if (xcab == 74 && ycab == 5) {
            WaitForSingleObject(semaforos[351], INFINITE);
        }
        if (xcab == 74 && ycab == 6) {
            WaitForSingleObject(semaforos[352], INFINITE);
        }
        if (xcab == 74 && ycab == 8) {
            WaitForSingleObject(semaforos[353], INFINITE);
        }
        if (xcab == 74 && ycab == 9) {
            WaitForSingleObject(semaforos[354], INFINITE);
        }
        if (xcab == 74 && ycab == 10) {
            WaitForSingleObject(semaforos[355], INFINITE);
        }
        if (xcab == 74 && ycab == 11) {
            WaitForSingleObject(semaforos[356], INFINITE);
        }
        if (xcab == 74 && ycab == 13) {
            WaitForSingleObject(semaforos[357], INFINITE);
        }
        if (xcab == 74 && ycab == 14) {
            WaitForSingleObject(semaforos[358], INFINITE);
        }
        if (xcab == 74 && ycab == 15) {
            WaitForSingleObject(semaforos[359], INFINITE);
        }



        /*##################### VERIFICACION DE AVANCE ######################*/

        WaitForSingleObject(semaforos[361], INFINITE);
        if (mapa[ycab][xcab] == 0) { //si no esta ocupado, lo ponemos ocupado
            mapa[ycab][xcab] = 1; //ponemos ocupado
        }
        ReleaseSemaphore(semaforos[361], 1, NULL);

        int coordsig = ycab;

        /*######################## DECIDE AVANZAR ######################*/
        WaitForSingleObject(semaforos[361], INFINITE);
        if (LOMO_avance(vectorIdTrenes[i], &xcola, &ycola) == -1) {
            pon_error("Error en la peticionAvance\n");
            terminarPrograma(1);
            return 100;
        }


        /*########### SIGNAL CRUCES #########################*/
        if (xcola == 16 && ycola == 0) {
            ReleaseSemaphore(semaforos[363], 1, NULL); //sem4
        }
        if (xcola == 36 && ycola == 0) {
            ReleaseSemaphore(semaforos[364], 1, NULL);
        }
        if (xcola == 54 && ycola == 0) {
            ReleaseSemaphore(semaforos[365], 1, NULL);
        }
        if (xcola == 68 && ycola == 0) {
            ReleaseSemaphore(semaforos[366], 1, NULL);
        }
        if (xcola == 74 && ycola == 0) {
            ReleaseSemaphore(semaforos[367], 1, NULL);
        }
        if (xcola == 16 && ycola == 4) {
            ReleaseSemaphore(semaforos[368], 1, NULL);
        }
        if (xcola == 16 && ycola == 7) {
            ReleaseSemaphore(semaforos[369], 1, NULL);
        }
        if (xcola == 36 && ycola == 7) {
            ReleaseSemaphore(semaforos[370], 1, NULL);
        }
        if (xcola == 54 && ycola == 7) {
            ReleaseSemaphore(semaforos[371], 1, NULL);
        }
        if (xcola == 68 && ycola == 7) {
            ReleaseSemaphore(semaforos[372], 1, NULL);
        }
        if (xcola == 74 && ycola == 7) {
            ReleaseSemaphore(semaforos[373], 1, NULL);
        }
        if (xcola == 0 && ycola == 9) {
            ReleaseSemaphore(semaforos[374], 1, NULL);
        }
        if (xcola == 16 && ycola == 9) {
            ReleaseSemaphore(semaforos[375], 1, NULL);
        }
        if (xcola == 0 && ycola == 12) {
            ReleaseSemaphore(semaforos[376], 1, NULL);
        }
        if (xcola == 16 && ycola == 12) {
            ReleaseSemaphore(semaforos[377], 1, NULL);
        }
        if (xcola == 36 && ycola == 12) {
            ReleaseSemaphore(semaforos[378], 1, NULL);
        }
        if (xcola == 54 && ycola == 12) {
            ReleaseSemaphore(semaforos[379], 1, NULL);
        }
        if (xcola == 68 && ycola == 12) {
            ReleaseSemaphore(semaforos[380], 1, NULL);
        }
        if (xcola == 74 && ycola == 12) {
            ReleaseSemaphore(semaforos[381], 1, NULL);
        }
        if (xcola == 0 && ycola == 16) {
            ReleaseSemaphore(semaforos[382], 1, NULL);
        }
        if (xcola == 16 && ycola == 16) {
            ReleaseSemaphore(semaforos[383], 1, NULL);
        }
        if (xcola == 36 && ycola == 16) {
            ReleaseSemaphore(semaforos[384], 1, NULL);
        }
        if (xcola == 54 && ycola == 16) {
            ReleaseSemaphore(semaforos[385], 1, NULL);
        }
        if (xcola == 68 && ycola == 16) {
            ReleaseSemaphore(semaforos[386], 1, NULL);
        }
        if (xcola == 74 && ycola == 16) {
            ReleaseSemaphore(semaforos[387], 1, NULL);
        }

        //trayectos
        //primer trayecto
        if (xcola == 0 && ycola == 0) {
            ReleaseSemaphore(semaforos[360], 1, NULL);
        }
        if (xcola == 1 && ycola == 0) {
            ReleaseSemaphore(semaforos[362], 1, NULL);
        }
        if (xcola == 2 && ycola == 0) {
            ReleaseSemaphore(semaforos[0], 1, NULL);
        }
        if (xcola == 3 && ycola == 0) {
            ReleaseSemaphore(semaforos[1], 1, NULL);
        }
        if (xcola == 4 && ycola == 0) {
            ReleaseSemaphore(semaforos[2], 1, NULL);
        }
        if (xcola == 5 && ycola == 0) {
            ReleaseSemaphore(semaforos[3], 1, NULL);
        }
        if (xcola == 6 && ycola == 0) {
            ReleaseSemaphore(semaforos[4], 1, NULL);
        }
        if (xcola == 7 && ycola == 0) {
            ReleaseSemaphore(semaforos[5], 1, NULL);
        }
        if (xcola == 8 && ycola == 0) {
            ReleaseSemaphore(semaforos[6], 1, NULL);
        }
        if (xcola == 9 && ycola == 0) {
            ReleaseSemaphore(semaforos[7], 1, NULL);
        }
        if (xcola == 10 && ycola == 0) {
            ReleaseSemaphore(semaforos[8], 1, NULL);
        }
        if (xcola == 11 && ycola == 0) {
            ReleaseSemaphore(semaforos[9], 1, NULL);
        }
        if (xcola == 12 && ycola == 0) {
            ReleaseSemaphore(semaforos[10], 1, NULL);
        }
        if (xcola == 13 && ycola == 0) {
            ReleaseSemaphore(semaforos[11], 1, NULL);
        }
        if (xcola == 14 && ycola == 0) {
            ReleaseSemaphore(semaforos[12], 1, NULL);
        }
        if (xcola == 15 && ycola == 0) {
            ReleaseSemaphore(semaforos[13], 1, NULL);
        }
        if (xcola == 17 && ycola == 0) {
            ReleaseSemaphore(semaforos[14], 1, NULL);
        }
        if (xcola == 18 && ycola == 0) {
            ReleaseSemaphore(semaforos[15], 1, NULL);
        }
        if (xcola == 19 && ycola == 0) {
            ReleaseSemaphore(semaforos[16], 1, NULL);
        }
        if (xcola == 20 && ycola == 0) {
            ReleaseSemaphore(semaforos[17], 1, NULL);
        }
        if (xcola == 21 && ycola == 0) {
            ReleaseSemaphore(semaforos[18], 1, NULL);
        }
        if (xcola == 22 && ycola == 0) {
            ReleaseSemaphore(semaforos[19], 1, NULL);
        }
        if (xcola == 23 && ycola == 0) {
            ReleaseSemaphore(semaforos[20], 1, NULL);
        }
        if (xcola == 24 && ycola == 0) {
            ReleaseSemaphore(semaforos[21], 1, NULL);
        }
        if (xcola == 25 && ycola == 0) {
            ReleaseSemaphore(semaforos[22], 1, NULL);
        }
        if (xcola == 26 && ycola == 0) {
            ReleaseSemaphore(semaforos[23], 1, NULL);
        }
        if (xcola == 27 && ycola == 0) {
            ReleaseSemaphore(semaforos[24], 1, NULL);
        }
        if (xcola == 28 && ycola == 0) {
            ReleaseSemaphore(semaforos[25], 1, NULL);
        }
        if (xcola == 29 && ycola == 0) {
            ReleaseSemaphore(semaforos[26], 1, NULL);
        }
        if (xcola == 30 && ycola == 0) {
            ReleaseSemaphore(semaforos[27], 1, NULL);
        }
        if (xcola == 31 && ycola == 0) {
            ReleaseSemaphore(semaforos[28], 1, NULL);
        }
        if (xcola == 32 && ycola == 0) {
            ReleaseSemaphore(semaforos[29], 1, NULL);
        }
        if (xcola == 33 && ycola == 0) {
            ReleaseSemaphore(semaforos[30], 1, NULL);
        }
        if (xcola == 34 && ycola == 0) {
            ReleaseSemaphore(semaforos[31], 1, NULL);
        }
        if (xcola == 35 && ycola == 0) {
            ReleaseSemaphore(semaforos[32], 1, NULL);
        }
        if (xcola == 37 && ycola == 0) {
            ReleaseSemaphore(semaforos[33], 1, NULL);
        }
        if (xcola == 38 && ycola == 0) {
            ReleaseSemaphore(semaforos[34], 1, NULL);
        }
        if (xcola == 39 && ycola == 0) {
            ReleaseSemaphore(semaforos[35], 1, NULL);
        }
        if (xcola == 40 && ycola == 0) {
            ReleaseSemaphore(semaforos[36], 1, NULL);
        }
        if (xcola == 41 && ycola == 0) {
            ReleaseSemaphore(semaforos[37], 1, NULL);
        }
        if (xcola == 42 && ycola == 0) {
            ReleaseSemaphore(semaforos[38], 1, NULL);
        }
        if (xcola == 43 && ycola == 0) {
            ReleaseSemaphore(semaforos[39], 1, NULL);
        }
        if (xcola == 44 && ycola == 0) {
            ReleaseSemaphore(semaforos[40], 1, NULL);
        }
        if (xcola == 45 && ycola == 0) {
            ReleaseSemaphore(semaforos[41], 1, NULL);
        }
        if (xcola == 46 && ycola == 0) {
            ReleaseSemaphore(semaforos[42], 1, NULL);
        }
        if (xcola == 47 && ycola == 0) {
            ReleaseSemaphore(semaforos[43], 1, NULL);
        }
        if (xcola == 48 && ycola == 0) {
            ReleaseSemaphore(semaforos[44], 1, NULL);
        }
        if (xcola == 49 && ycola == 0) {
            ReleaseSemaphore(semaforos[45], 1, NULL);
        }
        if (xcola == 50 && ycola == 0) {
            ReleaseSemaphore(semaforos[46], 1, NULL);
        }
        if (xcola == 51 && ycola == 0) {
            ReleaseSemaphore(semaforos[47], 1, NULL);
        }
        if (xcola == 52 && ycola == 0) {
            ReleaseSemaphore(semaforos[48], 1, NULL);
        }
        if (xcola == 53 && ycola == 0) {
            ReleaseSemaphore(semaforos[49], 1, NULL);
        }
        if (xcola == 55 && ycola == 0) {
            ReleaseSemaphore(semaforos[50], 1, NULL);
        }
        if (xcola == 56 && ycola == 0) {
            ReleaseSemaphore(semaforos[51], 1, NULL);
        }
        if (xcola == 57 && ycola == 0) {
            ReleaseSemaphore(semaforos[52], 1, NULL);
        }
        if (xcola == 58 && ycola == 0) {
            ReleaseSemaphore(semaforos[53], 1, NULL);
        }
        if (xcola == 59 && ycola == 0) {
            ReleaseSemaphore(semaforos[54], 1, NULL);
        }
        if (xcola == 60 && ycola == 0) {
            ReleaseSemaphore(semaforos[55], 1, NULL);
        }
        if (xcola == 61 && ycola == 0) {
            ReleaseSemaphore(semaforos[56], 1, NULL);
        }
        if (xcola == 62 && ycola == 0) {
            ReleaseSemaphore(semaforos[57], 1, NULL);
        }
        if (xcola == 63 && ycola == 0) {
            ReleaseSemaphore(semaforos[58], 1, NULL);
        }
        if (xcola == 64 && ycola == 0) {
            ReleaseSemaphore(semaforos[59], 1, NULL);
        }
        if (xcola == 65 && ycola == 0) {
            ReleaseSemaphore(semaforos[60], 1, NULL);
        }
        if (xcola == 66 && ycola == 0) {
            ReleaseSemaphore(semaforos[61], 1, NULL);
        }
        if (xcola == 67 && ycola == 0) {
            ReleaseSemaphore(semaforos[62], 1, NULL);
        }
        if (xcola == 69 && ycola == 0) {
            ReleaseSemaphore(semaforos[63], 1, NULL);
        }
        if (xcola == 70 && ycola == 0) {
            ReleaseSemaphore(semaforos[64], 1, NULL);
        }
        if (xcola == 71 && ycola == 0) {
            ReleaseSemaphore(semaforos[65], 1, NULL);
        }
        if (xcola == 72 && ycola == 0) {
            ReleaseSemaphore(semaforos[66], 1, NULL);
        }
        if (xcola == 73 && ycola == 0) {
            ReleaseSemaphore(semaforos[67], 1, NULL);
        }
        //segunda linea horizontal
        if (xcola == 0 && ycola == 4) {
            ReleaseSemaphore(semaforos[68], 1, NULL);
        }
        if (xcola == 1 && ycola == 4) {
            ReleaseSemaphore(semaforos[69], 1, NULL);
        }
        if (xcola == 2 && ycola == 4) {
            ReleaseSemaphore(semaforos[70], 1, NULL);
        }
        if (xcola == 3 && ycola == 4) {
            ReleaseSemaphore(semaforos[71], 1, NULL);
        }
        if (xcola == 4 && ycola == 4) {
            ReleaseSemaphore(semaforos[72], 1, NULL);
        }
        if (xcola == 5 && ycola == 4) {
            ReleaseSemaphore(semaforos[73], 1, NULL);
        }
        if (xcola == 6 && ycola == 4) {
            ReleaseSemaphore(semaforos[74], 1, NULL);
        }
        if (xcola == 7 && ycola == 4) {
            ReleaseSemaphore(semaforos[75], 1, NULL);
        }
        if (xcola == 8 && ycola == 4) {
            ReleaseSemaphore(semaforos[76], 1, NULL);
        }
        if (xcola == 9 && ycola == 4) {
            ReleaseSemaphore(semaforos[77], 1, NULL);
        }
        if (xcola == 10 && ycola == 4) {
            ReleaseSemaphore(semaforos[78], 1, NULL);
        }
        if (xcola == 11 && ycola == 4) {
            ReleaseSemaphore(semaforos[79], 1, NULL);
        }
        if (xcola == 12 && ycola == 4) {
            ReleaseSemaphore(semaforos[80], 1, NULL);
        }
        if (xcola == 13 && ycola == 4) {
            ReleaseSemaphore(semaforos[81], 1, NULL);
        }
        if (xcola == 14 && ycola == 4) {
            ReleaseSemaphore(semaforos[82], 1, NULL);
        }
        if (xcola == 15 && ycola == 4) {
            ReleaseSemaphore(semaforos[83], 1, NULL);
        }
        //tercera trayectoria horizontal
        if (xcola == 17 && ycola == 7) {
            ReleaseSemaphore(semaforos[84], 1, NULL);
        }
        if (xcola == 18 && ycola == 7) {
            ReleaseSemaphore(semaforos[85], 1, NULL);
        }
        if (xcola == 19 && ycola == 7) {
            ReleaseSemaphore(semaforos[86], 1, NULL);
        }
        if (xcola == 20 && ycola == 7) {
            ReleaseSemaphore(semaforos[87], 1, NULL);
        }
        if (xcola == 21 && ycola == 7) {
            ReleaseSemaphore(semaforos[88], 1, NULL);
        }
        if (xcola == 22 && ycola == 7) {
            ReleaseSemaphore(semaforos[89], 1, NULL);
        }
        if (xcola == 23 && ycola == 7) {
            ReleaseSemaphore(semaforos[90], 1, NULL);
        }
        if (xcola == 24 && ycola == 7) {
            ReleaseSemaphore(semaforos[91], 1, NULL);
        }
        if (xcola == 25 && ycola == 7) {
            ReleaseSemaphore(semaforos[92], 1, NULL);
        }
        if (xcola == 26 && ycola == 7) {
            ReleaseSemaphore(semaforos[93], 1, NULL);
        }
        if (xcola == 27 && ycola == 7) {
            ReleaseSemaphore(semaforos[94], 1, NULL);
        }
        if (xcola == 28 && ycola == 7) {
            ReleaseSemaphore(semaforos[95], 1, NULL);
        }
        if (xcola == 29 && ycola == 7) {
            ReleaseSemaphore(semaforos[96], 1, NULL);
        }
        if (xcola == 30 && ycola == 7) {
            ReleaseSemaphore(semaforos[97], 1, NULL);
        }
        if (xcola == 31 && ycola == 7) {
            ReleaseSemaphore(semaforos[98], 1, NULL);
        }
        if (xcola == 32 && ycola == 7) {
            ReleaseSemaphore(semaforos[99], 1, NULL);
        }
        if (xcola == 33 && ycola == 7) {
            ReleaseSemaphore(semaforos[100], 1, NULL);
        }
        if (xcola == 34 && ycola == 7) {
            ReleaseSemaphore(semaforos[101], 1, NULL);
        }
        if (xcola == 35 && ycola == 7) {
            ReleaseSemaphore(semaforos[102], 1, NULL);
        }
        if (xcola == 37 && ycola == 7) {
            ReleaseSemaphore(semaforos[103], 1, NULL);
        }
        if (xcola == 38 && ycola == 7) {
            ReleaseSemaphore(semaforos[104], 1, NULL);
        }
        if (xcola == 39 && ycola == 7) {
            ReleaseSemaphore(semaforos[105], 1, NULL);
        }
        if (xcola == 40 && ycola == 7) {
            ReleaseSemaphore(semaforos[106], 1, NULL);
        }
        if (xcola == 41 && ycola == 7) {
            ReleaseSemaphore(semaforos[107], 1, NULL);
        }
        if (xcola == 42 && ycola == 7) {
            ReleaseSemaphore(semaforos[108], 1, NULL);
        }
        if (xcola == 43 && ycola == 7) {
            ReleaseSemaphore(semaforos[109], 1, NULL);
        }
        if (xcola == 44 && ycola == 7) {
            ReleaseSemaphore(semaforos[110], 1, NULL);
        }
        if (xcola == 45 && ycola == 7) {
            ReleaseSemaphore(semaforos[111], 1, NULL);
        }
        if (xcola == 46 && ycola == 7) {
            ReleaseSemaphore(semaforos[112], 1, NULL);
        }
        if (xcola == 47 && ycola == 7) {
            ReleaseSemaphore(semaforos[113], 1, NULL);
        }
        if (xcola == 48 && ycola == 7) {
            ReleaseSemaphore(semaforos[114], 1, NULL);
        }
        if (xcola == 49 && ycola == 7) {
            ReleaseSemaphore(semaforos[115], 1, NULL);
        }
        if (xcola == 50 && ycola == 7) {
            ReleaseSemaphore(semaforos[116], 1, NULL);
        }
        if (xcola == 51 && ycola == 7) {
            ReleaseSemaphore(semaforos[117], 1, NULL);
        }
        if (xcola == 52 && ycola == 7) {
            ReleaseSemaphore(semaforos[118], 1, NULL);
        }
        if (xcola == 53 && ycola == 7) {
            ReleaseSemaphore(semaforos[119], 1, NULL);
        }
        if (xcola == 55 && ycola == 7) {
            ReleaseSemaphore(semaforos[120], 1, NULL);
        }
        if (xcola == 56 && ycola == 7) {
            ReleaseSemaphore(semaforos[121], 1, NULL);
        }
        if (xcola == 57 && ycola == 7) {
            ReleaseSemaphore(semaforos[122], 1, NULL);
        }
        if (xcola == 58 && ycola == 7) {
            ReleaseSemaphore(semaforos[123], 1, NULL);
        }
        if (xcola == 59 && ycola == 7) {
            ReleaseSemaphore(semaforos[124], 1, NULL);
        }
        if (xcola == 60 && ycola == 7) {
            ReleaseSemaphore(semaforos[125], 1, NULL);
        }
        if (xcola == 61 && ycola == 7) {
            ReleaseSemaphore(semaforos[126], 1, NULL);
        }
        if (xcola == 62 && ycola == 7) {
            ReleaseSemaphore(semaforos[127], 1, NULL);
        }
        if (xcola == 63 && ycola == 7) {
            ReleaseSemaphore(semaforos[128], 1, NULL);
        }
        if (xcola == 64 && ycola == 7) {
            ReleaseSemaphore(semaforos[129], 1, NULL);
        }
        if (xcola == 65 && ycola == 7) {
            ReleaseSemaphore(semaforos[130], 1, NULL);
        }
        if (xcola == 66 && ycola == 7) {
            ReleaseSemaphore(semaforos[131], 1, NULL);
        }
        if (xcola == 67 && ycola == 7) {
            ReleaseSemaphore(semaforos[132], 1, NULL);
        }
        if (xcola == 69 && ycola == 7) {
            ReleaseSemaphore(semaforos[133], 1, NULL);
        }
        if (xcola == 70 && ycola == 7) {
            ReleaseSemaphore(semaforos[134], 1, NULL);
        }
        if (xcola == 71 && ycola == 7) {
            ReleaseSemaphore(semaforos[135], 1, NULL);
        }
        if (xcola == 72 && ycola == 7) {
            ReleaseSemaphore(semaforos[136], 1, NULL);
        }
        if (xcola == 73 && ycola == 7) {
            ReleaseSemaphore(semaforos[137], 1, NULL);
        }
        //cuarta trayectoria horizontal
        if (xcola == 1 && ycola == 9) {
            ReleaseSemaphore(semaforos[138], 1, NULL);
        }
        if (xcola == 2 && ycola == 9) {
            ReleaseSemaphore(semaforos[139], 1, NULL);
        }
        if (xcola == 3 && ycola == 9) {
            ReleaseSemaphore(semaforos[140], 1, NULL);
        }
        if (xcola == 4 && ycola == 9) {
            ReleaseSemaphore(semaforos[141], 1, NULL);
        }
        if (xcola == 5 && ycola == 9) {
            ReleaseSemaphore(semaforos[142], 1, NULL);
        }
        if (xcola == 6 && ycola == 9) {
            ReleaseSemaphore(semaforos[143], 1, NULL);
        }
        if (xcola == 7 && ycola == 9) {
            ReleaseSemaphore(semaforos[144], 1, NULL);
        }
        if (xcola == 8 && ycola == 9) {
            ReleaseSemaphore(semaforos[145], 1, NULL);
        }
        if (xcola == 9 && ycola == 9) {
            ReleaseSemaphore(semaforos[146], 1, NULL);
        }
        if (xcola == 10 && ycola == 9) {
            ReleaseSemaphore(semaforos[147], 1, NULL);
        }
        if (xcola == 11 && ycola == 9) {
            ReleaseSemaphore(semaforos[148], 1, NULL);
        }
        if (xcola == 12 && ycola == 9) {
            ReleaseSemaphore(semaforos[149], 1, NULL);
        }
        if (xcola == 13 && ycola == 9) {
            ReleaseSemaphore(semaforos[150], 1, NULL);
        }
        if (xcola == 14 && ycola == 9) {
            ReleaseSemaphore(semaforos[151], 1, NULL);
        }
        if (xcola == 15 && ycola == 9) {
            ReleaseSemaphore(semaforos[152], 1, NULL);
        }
        //quinta trayectoria horizontal
        if (xcola == 1 && ycola == 12) {
            ReleaseSemaphore(semaforos[153], 1, NULL);
        }
        if (xcola == 2 && ycola == 12) {
            ReleaseSemaphore(semaforos[154], 1, NULL);
        }
        if (xcola == 3 && ycola == 12) {
            ReleaseSemaphore(semaforos[155], 1, NULL);
        }
        if (xcola == 4 && ycola == 12) {
            ReleaseSemaphore(semaforos[156], 1, NULL);
        }
        if (xcola == 5 && ycola == 12) {
            ReleaseSemaphore(semaforos[157], 1, NULL);
        }
        if (xcola == 6 && ycola == 12) {
            ReleaseSemaphore(semaforos[158], 1, NULL);
        }
        if (xcola == 7 && ycola == 12) {
            ReleaseSemaphore(semaforos[159], 1, NULL);
        }
        if (xcola == 8 && ycola == 12) {
            ReleaseSemaphore(semaforos[160], 1, NULL);
        }
        if (xcola == 9 && ycola == 12) {
            ReleaseSemaphore(semaforos[161], 1, NULL);
        }
        if (xcola == 10 && ycola == 12) {
            ReleaseSemaphore(semaforos[162], 1, NULL);
        }
        if (xcola == 11 && ycola == 12) {
            ReleaseSemaphore(semaforos[163], 1, NULL);
        }
        if (xcola == 12 && ycola == 12) {
            ReleaseSemaphore(semaforos[164], 1, NULL);
        }
        if (xcola == 13 && ycola == 12) {
            ReleaseSemaphore(semaforos[165], 1, NULL);
        }
        if (xcola == 14 && ycola == 12) {
            ReleaseSemaphore(semaforos[166], 1, NULL);
        }
        if (xcola == 15 && ycola == 12) {
            ReleaseSemaphore(semaforos[167], 1, NULL);
        }
        if (xcola == 17 && ycola == 12) {
            ReleaseSemaphore(semaforos[168], 1, NULL);
        }
        if (xcola == 18 && ycola == 12) {
            ReleaseSemaphore(semaforos[169], 1, NULL);
        }
        if (xcola == 19 && ycola == 12) {
            ReleaseSemaphore(semaforos[170], 1, NULL);
        }
        if (xcola == 20 && ycola == 12) {
            ReleaseSemaphore(semaforos[171], 1, NULL);
        }
        if (xcola == 21 && ycola == 12) {
            ReleaseSemaphore(semaforos[172], 1, NULL);
        }
        if (xcola == 22 && ycola == 12) {
            ReleaseSemaphore(semaforos[173], 1, NULL);
        }
        if (xcola == 23 && ycola == 12) {
            ReleaseSemaphore(semaforos[174], 1, NULL);
        }
        if (xcola == 24 && ycola == 12) {
            ReleaseSemaphore(semaforos[175], 1, NULL);
        }
        if (xcola == 25 && ycola == 12) {
            ReleaseSemaphore(semaforos[176], 1, NULL);
        }
        if (xcola == 26 && ycola == 12) {
            ReleaseSemaphore(semaforos[177], 1, NULL);
        }
        if (xcola == 27 && ycola == 12) {
            ReleaseSemaphore(semaforos[178], 1, NULL);
        }
        if (xcola == 28 && ycola == 12) {
            ReleaseSemaphore(semaforos[179], 1, NULL);
        }
        if (xcola == 29 && ycola == 12) {
            ReleaseSemaphore(semaforos[180], 1, NULL);
        }
        if (xcola == 30 && ycola == 12) {
            ReleaseSemaphore(semaforos[181], 1, NULL);
        }
        if (xcola == 31 && ycola == 12) {
            ReleaseSemaphore(semaforos[182], 1, NULL);
        }
        if (xcola == 32 && ycola == 12) {
            ReleaseSemaphore(semaforos[183], 1, NULL);
        }
        if (xcola == 33 && ycola == 12) {
            ReleaseSemaphore(semaforos[184], 1, NULL);
        }
        if (xcola == 34 && ycola == 12) {
            ReleaseSemaphore(semaforos[185], 1, NULL);
        }
        if (xcola == 35 && ycola == 12) {
            ReleaseSemaphore(semaforos[186], 1, NULL);
        }
        if (xcola == 37 && ycola == 12) {
            ReleaseSemaphore(semaforos[187], 1, NULL);
        }
        if (xcola == 38 && ycola == 12) {
            ReleaseSemaphore(semaforos[188], 1, NULL);
        }
        if (xcola == 39 && ycola == 12) {
            ReleaseSemaphore(semaforos[189], 1, NULL);
        }
        if (xcola == 40 && ycola == 12) {
            ReleaseSemaphore(semaforos[190], 1, NULL);
        }
        if (xcola == 41 && ycola == 12) {
            ReleaseSemaphore(semaforos[191], 1, NULL);
        }
        if (xcola == 42 && ycola == 12) {
            ReleaseSemaphore(semaforos[192], 1, NULL);
        }
        if (xcola == 43 && ycola == 12) {
            ReleaseSemaphore(semaforos[193], 1, NULL);
        }
        if (xcola == 44 && ycola == 12) {
            ReleaseSemaphore(semaforos[194], 1, NULL);
        }
        if (xcola == 45 && ycola == 12) {
            ReleaseSemaphore(semaforos[195], 1, NULL);
        }
        if (xcola == 46 && ycola == 12) {
            ReleaseSemaphore(semaforos[196], 1, NULL);
        }
        if (xcola == 47 && ycola == 12) {
            ReleaseSemaphore(semaforos[197], 1, NULL);
        }
        if (xcola == 48 && ycola == 12) {
            ReleaseSemaphore(semaforos[198], 1, NULL);
        }
        if (xcola == 49 && ycola == 12) {
            ReleaseSemaphore(semaforos[199], 1, NULL);
        }
        if (xcola == 50 && ycola == 12) {
            ReleaseSemaphore(semaforos[200], 1, NULL);
        }
        if (xcola == 51 && ycola == 12) {
            ReleaseSemaphore(semaforos[201], 1, NULL);
        }
        if (xcola == 52 && ycola == 12) {
            ReleaseSemaphore(semaforos[202], 1, NULL);
        }
        if (xcola == 53 && ycola == 12) {
            ReleaseSemaphore(semaforos[203], 1, NULL);
        }
        if (xcola == 55 && ycola == 12) {
            ReleaseSemaphore(semaforos[204], 1, NULL);
        }
        if (xcola == 56 && ycola == 12) {
            ReleaseSemaphore(semaforos[205], 1, NULL);
        }
        if (xcola == 57 && ycola == 12) {
            ReleaseSemaphore(semaforos[206], 1, NULL);
        }
        if (xcola == 58 && ycola == 12) {
            ReleaseSemaphore(semaforos[207], 1, NULL);
        }
        if (xcola == 59 && ycola == 12) {
            ReleaseSemaphore(semaforos[208], 1, NULL);
        }
        if (xcola == 60 && ycola == 12) {
            ReleaseSemaphore(semaforos[209], 1, NULL);
        }
        if (xcola == 61 && ycola == 12) {
            ReleaseSemaphore(semaforos[210], 1, NULL);
        }
        if (xcola == 62 && ycola == 12) {
            ReleaseSemaphore(semaforos[211], 1, NULL);
        }
        if (xcola == 63 && ycola == 12) {
            ReleaseSemaphore(semaforos[212], 1, NULL);
        }
        if (xcola == 64 && ycola == 12) {
            ReleaseSemaphore(semaforos[213], 1, NULL);
        }
        if (xcola == 65 && ycola == 12) {
            ReleaseSemaphore(semaforos[214], 1, NULL);
        }
        if (xcola == 66 && ycola == 12) {
            ReleaseSemaphore(semaforos[215], 1, NULL);
        }
        if (xcola == 67 && ycola == 12) {
            ReleaseSemaphore(semaforos[216], 1, NULL);
        }
        if (xcola == 69 && ycola == 12) {
            ReleaseSemaphore(semaforos[217], 1, NULL);
        }
        if (xcola == 70 && ycola == 12) {
            ReleaseSemaphore(semaforos[218], 1, NULL);
        }
        if (xcola == 71 && ycola == 12) {
            ReleaseSemaphore(semaforos[219], 1, NULL);
        }
        if (xcola == 72 && ycola == 12) {
            ReleaseSemaphore(semaforos[220], 1, NULL);
        }
        if (xcola == 73 && ycola == 12) {
            ReleaseSemaphore(semaforos[221], 1, NULL);
        }
        //sexta trayectoria horizontal
        if (xcola == 1 && ycola == 16) {
            ReleaseSemaphore(semaforos[222], 1, NULL);
        }
        if (xcola == 2 && ycola == 16) {
            ReleaseSemaphore(semaforos[223], 1, NULL);
        }
        if (xcola == 3 && ycola == 16) {
            ReleaseSemaphore(semaforos[224], 1, NULL);
        }
        if (xcola == 4 && ycola == 16) {
            ReleaseSemaphore(semaforos[225], 1, NULL);
        }
        if (xcola == 5 && ycola == 16) {
            ReleaseSemaphore(semaforos[226], 1, NULL);
        }
        if (xcola == 6 && ycola == 16) {
            ReleaseSemaphore(semaforos[227], 1, NULL);
        }
        if (xcola == 7 && ycola == 16) {
            ReleaseSemaphore(semaforos[228], 1, NULL);
        }
        if (xcola == 8 && ycola == 16) {
            ReleaseSemaphore(semaforos[229], 1, NULL);
        }
        if (xcola == 9 && ycola == 16) {
            ReleaseSemaphore(semaforos[230], 1, NULL);
        }
        if (xcola == 10 && ycola == 16) {
            ReleaseSemaphore(semaforos[231], 1, NULL);
        }
        if (xcola == 11 && ycola == 16) {
            ReleaseSemaphore(semaforos[232], 1, NULL);
        }
        if (xcola == 12 && ycola == 16) {
            ReleaseSemaphore(semaforos[233], 1, NULL);
        }
        if (xcola == 13 && ycola == 16) {
            ReleaseSemaphore(semaforos[234], 1, NULL);
        }
        if (xcola == 14 && ycola == 16) {
            ReleaseSemaphore(semaforos[235], 1, NULL);
        }
        if (xcola == 15 && ycola == 16) {
            ReleaseSemaphore(semaforos[236], 1, NULL);
        }
        if (xcola == 17 && ycola == 16) {
            ReleaseSemaphore(semaforos[237], 1, NULL);
        }
        if (xcola == 18 && ycola == 16) {
            ReleaseSemaphore(semaforos[238], 1, NULL);
        }
        if (xcola == 19 && ycola == 16) {
            ReleaseSemaphore(semaforos[239], 1, NULL);
        }
        if (xcola == 20 && ycola == 16) {
            ReleaseSemaphore(semaforos[240], 1, NULL);
        }
        if (xcola == 21 && ycola == 16) {
            ReleaseSemaphore(semaforos[241], 1, NULL);
        }
        if (xcola == 22 && ycola == 16) {
            ReleaseSemaphore(semaforos[242], 1, NULL);
        }
        if (xcola == 23 && ycola == 16) {
            ReleaseSemaphore(semaforos[243], 1, NULL);
        }
        if (xcola == 24 && ycola == 16) {
            ReleaseSemaphore(semaforos[244], 1, NULL);
        }
        if (xcola == 25 && ycola == 16) {
            ReleaseSemaphore(semaforos[245], 1, NULL);
        }
        if (xcola == 26 && ycola == 16) {
            ReleaseSemaphore(semaforos[246], 1, NULL);
        }
        if (xcola == 27 && ycola == 16) {
            ReleaseSemaphore(semaforos[247], 1, NULL);
        }
        if (xcola == 28 && ycola == 16) {
            ReleaseSemaphore(semaforos[248], 1, NULL);
        }
        if (xcola == 29 && ycola == 16) {
            ReleaseSemaphore(semaforos[249], 1, NULL);
        }
        if (xcola == 30 && ycola == 16) {
            ReleaseSemaphore(semaforos[250], 1, NULL);
        }
        if (xcola == 31 && ycola == 16) {
            ReleaseSemaphore(semaforos[251], 1, NULL);
        }
        if (xcola == 32 && ycola == 16) {
            ReleaseSemaphore(semaforos[252], 1, NULL);
        }
        if (xcola == 33 && ycola == 16) {
            ReleaseSemaphore(semaforos[253], 1, NULL);
        }
        if (xcola == 34 && ycola == 16) {
            ReleaseSemaphore(semaforos[254], 1, NULL);
        }
        if (xcola == 35 && ycola == 16) {
            ReleaseSemaphore(semaforos[255], 1, NULL);
        }
        if (xcola == 37 && ycola == 16) {
            ReleaseSemaphore(semaforos[256], 1, NULL);
        }
        if (xcola == 38 && ycola == 16) {
            ReleaseSemaphore(semaforos[257], 1, NULL);
        }
        if (xcola == 39 && ycola == 16) {
            ReleaseSemaphore(semaforos[258], 1, NULL);
        }
        if (xcola == 40 && ycola == 16) {
            ReleaseSemaphore(semaforos[259], 1, NULL);
        }
        if (xcola == 41 && ycola == 16) {
            ReleaseSemaphore(semaforos[260], 1, NULL);
        }
        if (xcola == 42 && ycola == 16) {
            ReleaseSemaphore(semaforos[261], 1, NULL);
        }
        if (xcola == 43 && ycola == 16) {
            ReleaseSemaphore(semaforos[262], 1, NULL);
        }
        if (xcola == 44 && ycola == 16) {
            ReleaseSemaphore(semaforos[263], 1, NULL);
        }
        if (xcola == 45 && ycola == 16) {
            ReleaseSemaphore(semaforos[264], 1, NULL);
        }
        if (xcola == 46 && ycola == 16) {
            ReleaseSemaphore(semaforos[265], 1, NULL);
        }
        if (xcola == 47 && ycola == 16) {
            ReleaseSemaphore(semaforos[266], 1, NULL);
        }
        if (xcola == 48 && ycola == 16) {
            ReleaseSemaphore(semaforos[267], 1, NULL);
        }
        if (xcola == 49 && ycola == 16) {
            ReleaseSemaphore(semaforos[268], 1, NULL);
        }
        if (xcola == 50 && ycola == 16) {
            ReleaseSemaphore(semaforos[269], 1, NULL);
        }
        if (xcola == 51 && ycola == 16) {
            ReleaseSemaphore(semaforos[270], 1, NULL);
        }
        if (xcola == 52 && ycola == 16) {
            ReleaseSemaphore(semaforos[271], 1, NULL);
        }
        if (xcola == 53 && ycola == 16) {
            ReleaseSemaphore(semaforos[272], 1, NULL);
        }
        if (xcola == 55 && ycola == 16) {
            ReleaseSemaphore(semaforos[273], 1, NULL);
        }
        if (xcola == 56 && ycola == 16) {
            ReleaseSemaphore(semaforos[274], 1, NULL);
        }
        if (xcola == 57 && ycola == 16) {
            ReleaseSemaphore(semaforos[275], 1, NULL);
        }
        if (xcola == 58 && ycola == 16) {
            ReleaseSemaphore(semaforos[276], 1, NULL);
        }
        if (xcola == 59 && ycola == 16) {
            ReleaseSemaphore(semaforos[277], 1, NULL);
        }
        if (xcola == 60 && ycola == 16) {
            ReleaseSemaphore(semaforos[278], 1, NULL);
        }
        if (xcola == 61 && ycola == 16) {
            ReleaseSemaphore(semaforos[279], 1, NULL);
        }
        if (xcola == 62 && ycola == 16) {
            ReleaseSemaphore(semaforos[280], 1, NULL);
        }
        if (xcola == 63 && ycola == 16) {
            ReleaseSemaphore(semaforos[281], 1, NULL);
        }
        if (xcola == 64 && ycola == 16) {
            ReleaseSemaphore(semaforos[282], 1, NULL);
        }
        if (xcola == 65 && ycola == 16) {
            ReleaseSemaphore(semaforos[283], 1, NULL);
        }
        if (xcola == 66 && ycola == 16) {
            ReleaseSemaphore(semaforos[284], 1, NULL);
        }
        if (xcola == 67 && ycola == 16) {
            ReleaseSemaphore(semaforos[285], 1, NULL);
        }
        if (xcola == 69 && ycola == 16) {
            ReleaseSemaphore(semaforos[286], 1, NULL);
        }
        if (xcola == 70 && ycola == 16) {
            ReleaseSemaphore(semaforos[287], 1, NULL);
        }
        if (xcola == 71 && ycola == 16) {
            ReleaseSemaphore(semaforos[288], 1, NULL);
        }
        if (xcola == 72 && ycola == 16) {
            ReleaseSemaphore(semaforos[289], 1, NULL);
        }
        if (xcola == 73 && ycola == 16) {
            ReleaseSemaphore(semaforos[290], 1, NULL);
        }

        //primer trayectoria vertical
        if (xcola == 0 && ycola == 4) {
            ReleaseSemaphore(semaforos[291], 1, NULL);
        }
        if (xcola == 0 && ycola == 5) {
            ReleaseSemaphore(semaforos[292], 1, NULL);
        }
        if (xcola == 0 && ycola == 6) {
            ReleaseSemaphore(semaforos[293], 1, NULL);
        }
        if (xcola == 0 && ycola == 7) {
            ReleaseSemaphore(semaforos[294], 1, NULL);
        }
        if (xcola == 0 && ycola == 8) {
            ReleaseSemaphore(semaforos[295], 1, NULL);
        }
        if (xcola == 0 && ycola == 10) {
            ReleaseSemaphore(semaforos[296], 1, NULL);
        }
        if (xcola == 0 && ycola == 11) {
            ReleaseSemaphore(semaforos[297], 1, NULL);
        }
        if (xcola == 0 && ycola == 13) {
            ReleaseSemaphore(semaforos[298], 1, NULL);
        }
        if (xcola == 0 && ycola == 14) {
            ReleaseSemaphore(semaforos[299], 1, NULL);
        }
        if (xcola == 0 && ycola == 15) {
            ReleaseSemaphore(semaforos[300], 1, NULL);
        }
        //segunda trayectoria vertical
        if (xcola == 16 && ycola == 1) {
            ReleaseSemaphore(semaforos[301], 1, NULL);
        }
        if (xcola == 16 && ycola == 2) {
            ReleaseSemaphore(semaforos[302], 1, NULL);
        }
        if (xcola == 16 && ycola == 3) {
            ReleaseSemaphore(semaforos[303], 1, NULL);
        }
        if (xcola == 16 && ycola == 5) {
            ReleaseSemaphore(semaforos[304], 1, NULL);
        }
        if (xcola == 16 && ycola == 6) {
            ReleaseSemaphore(semaforos[305], 1, NULL);
        }
        if (xcola == 16 && ycola == 8) {
            ReleaseSemaphore(semaforos[306], 1, NULL);
        }
        if (xcola == 16 && ycola == 10) {
            ReleaseSemaphore(semaforos[307], 1, NULL);
        }
        if (xcola == 16 && ycola == 11) {
            ReleaseSemaphore(semaforos[308], 1, NULL);
        }
        if (xcola == 16 && ycola == 13) {
            ReleaseSemaphore(semaforos[309], 1, NULL);
        }
        if (xcola == 16 && ycola == 14) {
            ReleaseSemaphore(semaforos[310], 1, NULL);
        }
        if (xcola == 16 && ycola == 15) {
            ReleaseSemaphore(semaforos[311], 1, NULL);
        }
        //tercera vertical
        if (xcola == 36 && ycola == 1) {
            ReleaseSemaphore(semaforos[312], 1, NULL);
        }
        if (xcola == 36 && ycola == 2) {
            ReleaseSemaphore(semaforos[313], 1, NULL);
        }
        if (xcola == 36 && ycola == 3) {
            ReleaseSemaphore(semaforos[314], 1, NULL);
        }
        if (xcola == 36 && ycola == 4) {
            ReleaseSemaphore(semaforos[388], 1, NULL);
        }
        if (xcola == 36 && ycola == 5) {
            ReleaseSemaphore(semaforos[315], 1, NULL);
        }
        if (xcola == 36 && ycola == 6) {
            ReleaseSemaphore(semaforos[316], 1, NULL);
        }
        if (xcola == 36 && ycola == 8) {
            ReleaseSemaphore(semaforos[317], 1, NULL);
        }
        if (xcola == 36 && ycola == 9) {
            ReleaseSemaphore(semaforos[318], 1, NULL);
        }
        if (xcola == 36 && ycola == 10) {
            ReleaseSemaphore(semaforos[319], 1, NULL);
        }
        if (xcola == 36 && ycola == 11) {
            ReleaseSemaphore(semaforos[320], 1, NULL);
        }
        if (xcola == 36 && ycola == 13) {
            ReleaseSemaphore(semaforos[321], 1, NULL);
        }
        if (xcola == 36 && ycola == 14) {
            ReleaseSemaphore(semaforos[322], 1, NULL);
        }
        if (xcola == 36 && ycola == 15) {
            ReleaseSemaphore(semaforos[323], 1, NULL);
        }
        //cuarta vertical
        if (xcola == 54 && ycola == 1) {
            ReleaseSemaphore(semaforos[324], 1, NULL);
        }
        if (xcola == 54 && ycola == 2) {
            ReleaseSemaphore(semaforos[325], 1, NULL);
        }
        if (xcola == 54 && ycola == 3) {
            ReleaseSemaphore(semaforos[326], 1, NULL);
        }
        if (xcola == 54 && ycola == 4) {
            ReleaseSemaphore(semaforos[389], 1, NULL);
        }
        if (xcola == 54 && ycola == 5) {
            ReleaseSemaphore(semaforos[327], 1, NULL);
        }
        if (xcola == 54 && ycola == 6) {
            ReleaseSemaphore(semaforos[328], 1, NULL);
        }
        if (xcola == 54 && ycola == 8) {
            ReleaseSemaphore(semaforos[329], 1, NULL);
        }
        if (xcola == 54 && ycola == 9) {
            ReleaseSemaphore(semaforos[330], 1, NULL);
        }
        if (xcola == 54 && ycola == 10) {
            ReleaseSemaphore(semaforos[331], 1, NULL);
        }
        if (xcola == 54 && ycola == 11) {
            ReleaseSemaphore(semaforos[332], 1, NULL);
        }
        if (xcola == 54 && ycola == 13) {
            ReleaseSemaphore(semaforos[333], 1, NULL);
        }
        if (xcola == 54 && ycola == 14) {
            ReleaseSemaphore(semaforos[334], 1, NULL);
        }
        if (xcola == 54 && ycola == 15) {
            ReleaseSemaphore(semaforos[335], 1, NULL);
        }
        //quinta vertical
        if (xcola == 68 && ycola == 1) {
            ReleaseSemaphore(semaforos[336], 1, NULL);
        }
        if (xcola == 68 && ycola == 2) {
            ReleaseSemaphore(semaforos[337], 1, NULL);
        }
        if (xcola == 68 && ycola == 3) {
            ReleaseSemaphore(semaforos[338], 1, NULL);
        }
        if (xcola == 68 && ycola == 4) {
            ReleaseSemaphore(semaforos[390], 1, NULL);
        }
        if (xcola == 68 && ycola == 5) {
            ReleaseSemaphore(semaforos[339], 1, NULL);
        }
        if (xcola == 68 && ycola == 6) {
            ReleaseSemaphore(semaforos[340], 1, NULL);
        }
        if (xcola == 68 && ycola == 8) {
            ReleaseSemaphore(semaforos[341], 1, NULL);
        }
        if (xcola == 68 && ycola == 9) {
            ReleaseSemaphore(semaforos[342], 1, NULL);
        }
        if (xcola == 68 && ycola == 10) {
            ReleaseSemaphore(semaforos[343], 1, NULL);
        }
        if (xcola == 68 && ycola == 11) {
            ReleaseSemaphore(semaforos[344], 1, NULL);
        }
        if (xcola == 68 && ycola == 13) {
            ReleaseSemaphore(semaforos[345], 1, NULL);
        }
        if (xcola == 68 && ycola == 14) {
            ReleaseSemaphore(semaforos[346], 1, NULL);
        }
        if (xcola == 68 && ycola == 15) {
            ReleaseSemaphore(semaforos[347], 1, NULL);
        }
        //ultima vertical
        if (xcola == 74 && ycola == 1) {
            ReleaseSemaphore(semaforos[348], 1, NULL);
        }
        if (xcola == 74 && ycola == 2) {
            ReleaseSemaphore(semaforos[349], 1, NULL);
        }
        if (xcola == 74 && ycola == 3) {
            ReleaseSemaphore(semaforos[350], 1, NULL);
        }
        if (xcola == 74 && ycola == 4) {
            ReleaseSemaphore(semaforos[391], 1, NULL);
        }
        if (xcola == 74 && ycola == 5) {
            ReleaseSemaphore(semaforos[351], 1, NULL);
        }
        if (xcola == 74 && ycola == 6) {
            ReleaseSemaphore(semaforos[352], 1, NULL);
        }
        if (xcola == 74 && ycola == 8) {
            ReleaseSemaphore(semaforos[353], 1, NULL);
        }
        if (xcola == 74 && ycola == 9) {
            ReleaseSemaphore(semaforos[354], 1, NULL);
        }
        if (xcola == 74 && ycola == 10) {
            ReleaseSemaphore(semaforos[355], 1, NULL);
        }
        if (xcola == 74 && ycola == 11) {
            ReleaseSemaphore(semaforos[356], 1, NULL);
        }
        if (xcola == 74 && ycola == 13) {
            ReleaseSemaphore(semaforos[357], 1, NULL);
        }
        if (xcola == 74 && ycola == 14) {
            ReleaseSemaphore(semaforos[358], 1, NULL);
        }
        if (xcola == 74 && ycola == 15) {
            ReleaseSemaphore(semaforos[359], 1, NULL);
        }

        //para evitar que entre aqui mientras esta saliendo el tren
        if (xcola != -1 && ycola != -1) {// sino se libera posicion,nos devuelve un -1
            mapa[ycola][xcola] = 0;
        }


        LOMO_espera(coordsig - 1, coordsig);
        ReleaseSemaphore(semaforos[361], 1, NULL);
        WaitForSingleObject(semaforos[361], INFINITE);

        //TRAYECTORIA1
        if (mapa[4][0] == 1 && mapa[4][1] == 1 && mapa[4][2] == 1 && mapa[4][3] == 1 && mapa[4][4] == 1 && mapa[4][5] == 1 && mapa[4][6] == 1 && mapa[4][7] == 1 && mapa[4][8] == 1 && mapa[4][9] == 1 && mapa[4][10] == 1 && mapa[4][11] == 1 && mapa[4][12] == 1 && mapa[4][13] == 1 && mapa[4][14] == 1 && mapa[4][15] == 1 && mapa[4][16] == 1) {
            if (mapa[9][0] == 1 && mapa[9][1] == 1 && mapa[9][2] == 1 && mapa[9][3] == 1 && mapa[9][4] == 1 && mapa[9][5] == 1 && mapa[9][6] == 1 && mapa[9][7] == 1 && mapa[9][8] == 1 && mapa[9][9] == 1 && mapa[9][10] == 1 && mapa[9][11] == 1 && mapa[9][12] == 1 && mapa[9][13] == 1 && mapa[9][14] == 1 && mapa[9][15] == 1 && mapa[9][16] == 1) {
                if (mapa[5][0] == 1 && mapa[6][0] == 1 && mapa[7][0] == 1 && mapa[8][0] == 1) {
                    if (mapa[5][16] == 1 && mapa[6][16] == 1 && mapa[7][16] == 1 && mapa[8][16] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }

            }

        }
        //TRAYECTORIA2
        if (mapa[9][0] == 1 && mapa[9][1] == 1 && mapa[9][2] == 1 && mapa[9][3] == 1 && mapa[9][4] == 1 && mapa[9][5] == 1 && mapa[9][6] == 1 && mapa[9][7] == 1 && mapa[9][8] == 1 && mapa[9][9] == 1 && mapa[9][10] == 1 && mapa[9][11] == 1 && mapa[9][12] == 1 && mapa[9][13] == 1 && mapa[9][14] == 1 && mapa[9][15] == 1 && mapa[9][16] == 1) {
            if (mapa[12][0] == 1 && mapa[12][1] == 1 && mapa[12][2] == 1 && mapa[12][3] == 1 && mapa[12][4] == 1 && mapa[12][5] == 1 && mapa[12][6] == 1 && mapa[12][7] == 1 && mapa[12][8] == 1 && mapa[12][9] == 1 && mapa[12][10] == 1 && mapa[12][11] == 1 && mapa[12][12] == 1 && mapa[12][13] == 1 && mapa[12][14] == 1 && mapa[12][15] == 1 && mapa[12][16] == 1) {
                if (mapa[10][0] == 1 && mapa[11][0] == 1) {
                    if (mapa[10][16] == 1 && mapa[11][16] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }
        //TRAYECTORIA3
        if (mapa[12][0] == 1 && mapa[12][1] == 1 && mapa[12][2] == 1 && mapa[12][3] == 1 && mapa[12][4] == 1 && mapa[12][5] == 1 && mapa[12][6] == 1 && mapa[12][7] == 1 && mapa[12][8] == 1 && mapa[12][9] == 1 && mapa[12][10] == 1 && mapa[12][11] == 1 && mapa[12][12] == 1 && mapa[12][13] == 1 && mapa[12][14] == 1 && mapa[12][15] == 1 && mapa[12][16] == 1) {
            if (mapa[16][0] == 1 && mapa[16][1] == 1 && mapa[16][2] == 1 && mapa[16][3] == 1 && mapa[16][4] == 1 && mapa[16][5] == 1 && mapa[16][6] == 1 && mapa[16][7] == 1 && mapa[16][8] == 1 && mapa[16][9] == 1 && mapa[16][10] == 1 && mapa[16][11] == 1 && mapa[16][12] == 1 && mapa[16][13] == 1 && mapa[16][14] == 1 && mapa[16][15] == 1 && mapa[16][16] == 1) {
                if (mapa[13][0] == 1 && mapa[14][0] == 1 && mapa[15][0] == 1) {
                    if (mapa[13][16] == 1 && mapa[14][16] == 1 && mapa[15][16] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }
        //TRAYECTORIA1_2
        if (mapa[4][0] == 1 && mapa[4][1] == 1 && mapa[4][2] == 1 && mapa[4][3] == 1 && mapa[4][4] == 1 && mapa[4][5] == 1 && mapa[4][6] == 1 && mapa[4][7] == 1 && mapa[4][8] == 1 && mapa[4][9] == 1 && mapa[4][10] == 1 && mapa[4][11] == 1 && mapa[4][12] == 1 && mapa[4][13] == 1 && mapa[4][14] == 1 && mapa[4][15] == 1 && mapa[4][16] == 1) {
            if (mapa[12][0] == 1 && mapa[12][1] == 1 && mapa[12][2] == 1 && mapa[12][3] == 1 && mapa[12][4] == 1 && mapa[12][5] == 1 && mapa[12][6] == 1 && mapa[12][7] == 1 && mapa[12][8] == 1 && mapa[12][9] == 1 && mapa[12][10] == 1 && mapa[12][11] == 1 && mapa[12][12] == 1 && mapa[12][13] == 1 && mapa[12][14] == 1 && mapa[12][15] == 1 && mapa[12][16] == 1) {
                if (mapa[5][0] == 1 && mapa[6][0] == 1 && mapa[7][0] == 1 && mapa[8][0] == 1 && mapa[10][0] == 1 && mapa[11][0] == 1) {
                    if (mapa[5][16] == 1 && mapa[6][16] == 1 && mapa[7][16] == 1 && mapa[8][16] == 1 && mapa[10][16] == 1 && mapa[11][16] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }
        //TRAYECTORIA2_3
        if (mapa[9][0] == 1 && mapa[9][1] == 1 && mapa[9][2] == 1 && mapa[9][3] == 1 && mapa[9][4] == 1 && mapa[9][5] == 1 && mapa[9][6] == 1 && mapa[9][7] == 1 && mapa[9][8] == 1 && mapa[9][9] == 1 && mapa[9][10] == 1 && mapa[9][11] == 1 && mapa[9][12] == 1 && mapa[9][13] == 1 && mapa[9][14] == 1 && mapa[9][15] == 1 && mapa[9][16] == 1) {
            if (mapa[16][0] == 1 && mapa[16][1] == 1 && mapa[16][2] == 1 && mapa[16][3] == 1 && mapa[16][4] == 1 && mapa[16][5] == 1 && mapa[16][6] == 1 && mapa[16][7] == 1 && mapa[16][8] == 1 && mapa[16][9] == 1 && mapa[16][10] == 1 && mapa[16][11] == 1 && mapa[16][12] == 1 && mapa[16][13] == 1 && mapa[16][14] == 1 && mapa[16][15] == 1 && mapa[16][16] == 1) {
                if (mapa[10][0] == 1 && mapa[11][0] == 1 && mapa[13][0] == 1 && mapa[14][0] == 1 && mapa[15][0] == 1) {
                    if (mapa[10][16] == 1 && mapa[11][16] == 1 && mapa[13][16] == 1 && mapa[14][16] == 1 && mapa[15][16] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }
        //TRAYECTORIA13
        if (mapa[0][68] == 1 && mapa[0][69] == 1 && mapa[0][70] == 1 && mapa[0][71] == 1 && mapa[0][72] == 1 && mapa[0][73] == 1 && mapa[0][74] == 1 ) {
            if (mapa[1][68] == 1 && mapa[2][68] == 1 && mapa[3][68] == 1 && mapa[4][68] == 1 && mapa[5][68] == 1 && mapa[6][68] == 1 ) {
                if (mapa[7][68] == 1 && mapa[7][69] == 1 && mapa[7][70] == 1 && mapa[7][71] == 1 && mapa[7][72] == 1 && mapa[7][73] == 1 && mapa[7][74] == 1) {
                    if (mapa[1][74] == 1 && mapa[2][74] == 1 && mapa[3][74] == 1 && mapa[4][74] == 1 && mapa[5][74] == 1 && mapa[6][74] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }

        //TRAYECTORIA14
        if (mapa[7][68] == 1 && mapa[7][69] == 1 && mapa[7][70] == 1 && mapa[7][71] == 1 && mapa[7][72] == 1 && mapa[7][73] == 1 && mapa[7][74] == 1) {
            if (mapa[12][68] == 1 && mapa[12][69] == 1 && mapa[12][70] == 1 && mapa[12][71] == 1 && mapa[12][72] == 1 && mapa[12][73] == 1 && mapa[12][74] == 1) {
                if (mapa[8][68] == 1 && mapa[9][69] == 1 && mapa[10][68] == 1 && mapa[11][68] == 1) {
                    if (mapa[8][74] == 1 && mapa[9][74] == 1 && mapa[10][74] == 1 && mapa[11][74] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }

        //TRAYECTORIA15
        if (mapa[12][68] == 1 && mapa[12][69] == 1 && mapa[12][70] == 1 && mapa[12][71] == 1 && mapa[12][72] == 1 && mapa[12][73] == 1 && mapa[12][74] == 1) {
            if (mapa[16][68] == 1 && mapa[16][69] == 1 && mapa[16][70] == 1 && mapa[16][71] == 1 && mapa[16][72] == 1 && mapa[16][73] == 1 && mapa[16][74] == 1) {
                if (mapa[13][68] == 1 && mapa[14][68] == 1 && mapa[15][68] == 1) {
                    if (mapa[13][74] == 1 && mapa[14][74] == 1 && mapa[15][74] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }
        }

        //TRAYECTORIA13_14
        if (mapa[0][68] == 1 && mapa[0][69] != 0 && mapa[0][70] != 0 && mapa[0][71] != 0 && mapa[0][72] != 0 && mapa[0][73] != 0 && mapa[0][74] != 0) {
            if (mapa[12][68] != 0 && mapa[12][69] != 0 && mapa[12][70] != 0 && mapa[12][71] != 0 && mapa[12][72] == 1 && mapa[12][73] == 1 && mapa[12][74] == 1) {
                if (mapa[1][68] == 1 && mapa[2][68] == 1 && mapa[3][68] == 1 && mapa[4][68] == 1 && mapa[5][68] == 1 && mapa[6][68] == 1 && mapa[7][68] == 1 && mapa[8][68] == 1 && mapa[9][68] == 1 && mapa[10][68] == 1 && mapa[11][68] == 1 && mapa[12][68] == 1) {
                    if (mapa[1][74] == 1 && mapa[2][74] == 1 && mapa[3][74] == 1 && mapa[4][74] == 1 && mapa[5][74] == 1 && mapa[6][74] == 1 && mapa[7][74] == 1 && mapa[8][74] == 1 && mapa[9][74] == 1 && mapa[10][74] == 1 && mapa[11][74] == 1 && mapa[12][74] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }
                }
            }

        }
        //TRAYECTORIA14_15
        if (mapa[7][68] == 1 && mapa[7][69] != 0 && mapa[7][70] != 0 && mapa[7][71] != 0 && mapa[7][72] != 0 && mapa[7][73] != 0 && mapa[7][74] != 0) {
            if (mapa[16][68] == 1 && mapa[16][69] != 0 && mapa[16][70] != 0 && mapa[16][71] != 0 && mapa[16][72] != 0 && mapa[16][73] != 0 && mapa[16][74] != 0) {
                if (mapa[7][68] == 1 && mapa[8][68] == 1 && mapa[9][68] == 1 && mapa[10][68] == 1 && mapa[11][68] == 1 && mapa[12][68] == 1 && mapa[13][68] == 1 && mapa[14][68] == 1 && mapa[15][68] == 1 && mapa[16][68] == 1) {
                    if (mapa[7][74] == 1 && mapa[8][74] == 1 && mapa[9][74] == 1 && mapa[10][74] == 1 && mapa[11][74] == 1 && mapa[12][74] == 1 && mapa[13][74] == 1 && mapa[14][74] == 1 && mapa[15][74] == 1 && mapa[16][74] == 1) {
                        pon_error("INTERBLOQUEO");
                        terminarPrograma(1);
                        return 100;
                    }

                }
            }
        }


        ReleaseSemaphore(semaforos[361], 1, NULL);
    }//fin bucle while


    //Cada hijo hace un signal hasta desbloquear el recurso
    if (ReleaseSemaphore(sem_EsperarPadre, 1, NULL) == FALSE) {
        pon_error("Error signal EsperarPadre\n");
        terminarPrograma(1);
        return 100;
    }

}

void terminarPrograma(int signal) {

    if (GetCurrentThread() == trenPadre) {
        if (LOMO_fin() == -1) {
            fprintf(stderr, "Error al finalizar la practica\n");
            exit(1);
        }
        system("cls");


        //liberamos libreria
        if (FreeLibrary(libreria) == FALSE) {
            pon_error("ERROR liberacion biblioteca\n");
        }

        //liberamos los handle de los trenes
        for (int i = 0; i < nTrenes; i++) {
            if (CloseHandle(vectorTrenes[i]) == FALSE) {
                fprintf(stderr, "Error, no se pudo cerrar vectorTrenes\n");
                exit(1);
            }
        }

        if (sem_EsperarPadre != NULL) {
            if (CloseHandle(sem_EsperarPadre) == FALSE) {
                fprintf(stderr, "Error, no se pudo cerrar semEsperarPadre\n");
                exit(1);
            }
        }

        //liberamos semaforos de las coordenadas
        for (int i = 0; i < 392; i++) {
            if (CloseHandle(semaforos[i]) == FALSE) {
                fprintf(stderr, "Error, no se pudo cerrar semaforos\n");
                exit(1);
            }
    
        }


        if (CloseHandle(trenPadre) == FALSE) {
            fprintf(stderr, "Error, no se pudo cerrar trenPadre\n");
            exit(1);
        }
        

        fprintf(stderr, "La practica ha finalizado correctamente\n");
        exit(0);
    }
    else {
        exit(0);
    }

}

//control c
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
        terminarPrograma(1);
        return TRUE;
    default:
        return FALSE;
    }
}
