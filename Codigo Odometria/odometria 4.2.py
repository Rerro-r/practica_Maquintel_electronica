import csv
import datetime
import os
import serial
import time
import tkinter as tk
from tkinter import ttk


Estado = 0
Estado_reset = 0


def puertos_seriales():
    ports = [f"COM{i + 1}" for i in range(24)]
    encontrados = []
    for port in ports:
        try:
            with serial.Serial(port) as s:
                encontrados.append(port)
        except (OSError, serial.SerialException):
            pass
    return encontrados


def desconectar():
    global Estado
    Estado = 0


def reset():
    global Estado
    global Estado_reset
    if Estado == 1:
        Estado_reset = 1


def conexion():
# Obtiene la hora actual y la utiliza para nombrar el archivo CSV
    hora_actual = datetime.datetime.now().strftime("%H.%M.%S.%f")
    nombre_archivo = f"C:\\Users\\Renate\\Desktop\\Practica_Maquintel\\Codigo Odometria\\CSVs de prueba\\{hora_actual}.csv"

    # Inicializa las variables globales
    global port_lista, Estado, Estado_reset, selec_imu
    Estado = 1

    # Crea el archivo CSV
    with open(nombre_archivo, "w", newline="") as file:
        writer = csv.writer(file, delimiter=";", quotechar='"', quoting=csv.QUOTE_NONNUMERIC)
        writer.writerow(["Hora", "Distancia", "GIROSCOPIO", "ACELEROMETRO", "MAGNETROMETRO"])

    # Inicializa la conexión serial
    if port_lista.get() and odometro_lista.get():
        if selec_imu.get():
            # Inicializa la conexión serial para la IMU
            puertoSerial = serial.Serial(port_lista2.get(), 115200, timeout=2)
            puertoSerial.write("iniciar".encode())
            print("Conexión con IMU establecida")
            
            # Inicializa la conexión serial para el software sonar
            puertoSerial_b = serial.Serial("COM29", 115200, timeout=2)
            puertoSerial_b.write("iniciar".encode())
            print("Conexión con software sonar establecida")
        
        # Inicializa la conexión serial para la odometría
        puertoSerial_c = serial.Serial(port_lista.get(),115200, timeout=2)
        #time.sleep(0.2)
        #puertoSerial_c.flushInput()
        
    else:
        # Si falta algún parámetro para la conexión serial, cambia el estado a 0
        Estado = 0

    Ti=""
    pitch="+00.0"
    roll="+000.0"
    Distancia= "000.01"
    acelerometro=[0,0,0,0,0,0]
    magnetometro=[0,0,0,0,0,0]
    giroscopio=[0,0,0,0,0,0]
            #print("transmitiendo data..")

            #puertoSerial.flushInput()
    while Estado==1:
        time.sleep(0.028) #tiempo de muestro
        ace=" "
        giro=" "
        mag=" "
        Ti=""
        salto_de_linea=0
        imu=""   
        if selec_imu.get()==1:
                        while salto_de_linea<3: 
                            if puertoSerial.in_waiting>0:
                                lectura = puertoSerial.readline()
                                if len(lectura)>5:
                                    imu= imu+lectura.decode('cp1252')
                                    salto_de_linea=salto_de_linea+1
                                    time.sleep(0.002) #tiempo de muestro
                        puertoSerial.write("OK".encode('utf-8'))
                        #print(imu)
                        if imu!="":
                            
                            lista_IMU=imu.split('\n') 
                            
                            #print(lista_IMU)
                            if len(lista_IMU)>=3:
                                #se separan los datos por cada sensor segun lo indicado por el indice de la trama
                                for i in lista_IMU:
                                    if len(i)>6:
                                        A=i.split(",")
                                        #print(A[1])
                                        if len(A)>=6:
                                            #print(len(A))
                                            if A[1] == "0":
                                                giroscopio=i.split(",")
                                            elif A[1] =="1":
                                                acelerometro=i.split(",") 
                                            elif A[1] =="2":
                                                magnetometro=i.split(",")

                                # se subdividen los datos 

                                ace= str(acelerometro[1]) + ";"+ str(acelerometro[3]) +";" +str(acelerometro[4])+";"+ str(acelerometro[5])
                                giro=str(giroscopio[1]) + ";"+ str(giroscopio[3]) +";" +str(giroscopio[4])+";"+ str(giroscopio[5])
                                mag= str(magnetometro[1]) + ";"+ str(magnetometro[3]) +";" +str(magnetometro[4])+";"+ str(magnetometro[5]) 
        if puertoSerial_c.in_waiting>0:
            Tics = puertoSerial_c.readline()
            if len(Tics) > 0:
                    Ti = "".join(filter(lambda x: x.isdigit(), str(Tics)))
                    if odometro_lista.get() == "Guia de cable":
                        Distancia = round((((int(Ti) * 0.0372 * 3.1416) / 1024) * 1), 2)
                    elif odometro_lista.get() == "Carrete":
                        Distancia = round((((int(Ti) * 0.0225 * 3.1416) / 1024) * 1.0216), 2)
                    string_DIstancia=str(Distancia).split(".")
                    if len(string_DIstancia[1])<2:
                            string_DIstancia[1]= string_DIstancia[1]+"0"
                    Distancia=string_DIstancia[0]+"."+string_DIstancia[1]

                    #print(Distancia)

                    if float(Distancia)>=0:
                            #print(len(str(Distancia)))
                            if len(str(Distancia))==4:
                                Distancia="+000" + str(Distancia)
                            if len(str(Distancia))==5:
                                Distancia="+00" + str(Distancia)
                            if len(str(Distancia))==6:
                                Distancia="+0" + str(Distancia)
                            if len(str(Distancia))==7:
                                Distancia="+" + str(Distancia)
                    else:
                            Distancia=str(Distancia)
                            if len(str(Distancia))==5:
                                Distancia=Distancia[ :1]+"000"+Distancia[1: ]
                            if len(str(Distancia))==6:
                                Distancia=Distancia[ :1]+"00"+Distancia[1: ]
                            if len(str(Distancia))==7:
                                Distancia=Distancia[ :1]+"0"+Distancia[1: ]                 
        #print(Distancia)

        formato = f"$PITCH{pitch},ROLL{roll},DIST{Distancia}\r\n"
        #ruta_archivo = f"C:/Users/seba_/OneDrive/Documentos/Odometrias/{nombre_archivo}"
        with open(nombre_archivo, 'a', newline='') as archivo:
                escritor = csv.writer(archivo, delimiter=';', quotechar='"', quoting=csv.QUOTE_NONNUMERIC)
                hora_actual = datetime.datetime.now()
                escritor.writerow([hora_actual.time(), Distancia, giro, ace, mag])
        if selec_imu.get() == 1:
                puertoSerial_b.write(formato.encode('utf-8'))
        dis.set(Distancia)
        if Estado_reset == 1:
            nueva_dis="reset"+input_.get()+"@"
            #print(nueva_dis)
            puertoSerial_c.write(nueva_dis.encode('utf-8'))
            Estado_reset=0
        ventana.update()
    if selec_imu == 1:
            puertoSerial.flushInput()
            puertoSerial.close()
            puertoSerial_b.close()
    if port_lista.get() and odometro_lista.get():
            puertoSerial_c.flushInput()
            puertoSerial_c.close()
        
ventana = tk.Tk()
ventana.title("Odometria - Maquintel")
ventana.geometry('350x200')
ventana.configure(background='dark orange')

logo = tk.PhotoImage(file=f"C:\\Users\\Renate\\Desktop\\Practica_Maquintel\\Codigo Odometria\\Logos_Maquintel\\Logos_Maquintel.png")
label = tk.Label(ventana, image=logo, background='dark orange', width=300)
label.place(x=20, y=0)

etiqueta1 = tk.Label(ventana, text='Puerto: ', bg='white', fg='black')
etiqueta1.place(x=5, y=80)

dis = tk.StringVar(value="000.00")
etiqueta2 = tk.Label(ventana, text='Distancia: ', bg='white', fg='black')
etiqueta2.place(x=5, y=159)
etiqueta3 = tk.Label(ventana, textvariable=dis, bg='white', fg='black', width=9)
etiqueta3.place(x=80, y=159)

port_lista = ttk.Combobox(ventana, width=10, values=puertos_seriales())
port_lista.place(x=60, y=80)

port_lista2 = ttk.Combobox(ventana, width=10, values=puertos_seriales())
port_lista2.place(x=60, y=115)

b_desconectar = tk.Button(ventana, text='Desconectar', command=desconectar, width=9)
b_desconectar.place(x=155, y=115)

b_conectar = tk.Button(ventana, text='Conectar', command=conexion, width=9)
b_conectar.place(x=155, y=78)

b_reset = tk.Button(ventana, text='Reset a: ', command=reset)
b_reset.place(x=155, y=155)

input_ = tk.Entry(ventana, width=15)
input_.place(x=225, y=159)

odometro_lista = ttk.Combobox(ventana, width=10, state="readonly", values=['Guia de cable', 'Carrete'])
odometro_lista.place(x=250, y=105)

etiqueta4 = tk.Label(ventana, text='Tipo odometro', bg='white', fg='black', width=13)
etiqueta4.place(x=245, y=80)

selec_imu = tk.IntVar()
check = tk.Checkbutton(ventana, variable=selec_imu, onvalue=1, offvalue=0, bg="dark orange", activebackground="dark orange", text='IMU')
check.place(x=5, y=115)
print("hola")
ventana.mainloop()