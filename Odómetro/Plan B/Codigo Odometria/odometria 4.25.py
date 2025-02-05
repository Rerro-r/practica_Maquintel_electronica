import csv
import datetime
import os
import serial
import time
import tkinter as tk
from tkinter import ttk

Estado = 0
Estado_reset = 0
envio_encoder = False  # Debe ser True cada vez que se desconecta de forma manual o accidental el receptor
begin_reset = 0
envio_encoder_listo = False
Ticks = 0
ti = 0
ti_ant = 0
puertoSerial_c = None

current_file_path = os.path.abspath(__file__)
current_folder = os.path.dirname(current_file_path)
logo_path = os.path.join(current_folder, "Logos_Maquintel", "Logos_Maquintel.png")

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

def actualizar_puertos():
    """Actualiza la lista de puertos COM si no hay conexión establecida."""
    if Estado == 0:
        puertos_disponibles = puertos_seriales()
        port_lista["values"] = puertos_disponibles
        port_lista2["values"] = puertos_disponibles
    ventana.after(1000, actualizar_puertos)  # Repite cada 1000 ms (1 segundo)

def desconectar():
    global Estado, envio_encoder, puertoSerial_c
    Estado = 0
    envio_encoder = True
    port_lista.config(state="normal")
    port_lista2.config(state="normal")
    odometro_lista.config(state="normal")
    input_reset.config(state="disabled")
    input_ratio.config(state="normal")
    selec_imu.set(0)
    b_conectar.config(state="normal")
    b_reset.config(state="disabled")
    b_desconectar.config(state="disabled")
    mensaje_error.config(text="")
    check.config(state="normal")
    mensaje = "STOP"
    print(mensaje)
    if puertoSerial_c != None:
        puertoSerial_c.write(mensaje.encode('utf-8'))

def reset():
    global Estado_reset, begin_reset, puertoSerial_c, Ticks
    mensaje_error.config(text="")
    if Estado == 1:
        try:
            # Intentar convertir el valor ingresado a float
            begin_reset = float(input_reset.get())  # Si es válido, se guarda en begin_reset
            Estado_reset = 1  # Si la conversión es exitosa, se activa el reset
            if odometro_lista.get() == "Guia de cable":
                Ticks = round((begin_reset * 1024) / (0.0372 * 3.1416))
            elif odometro_lista.get() == "Carrete":
                Ticks = round((begin_reset * 1024) / (0.0225 * 3.1416 * 1.0216))
            elif odometro_lista.get() == "Personalizado":
                Ticks = round((begin_reset * 1024) / (float(input_ratio.get()) * 3.1416))
        except ValueError:
            # Si ocurre un error al intentar convertir a float, muestra un mensaje de error
            mensaje_error.config(text="Error: El valor ingresado no es válido. Ingrese un número válido. Ejemplo: 0.05. Use punto para los decimales")

def conexion():
    global Estado, envio_encoder, Estado_reset, port_lista, selec_imu, puertoSerial_c, envio_encoder_listo, Ticks, ti_ant
    mensaje_error.config(text="")
    Estado = 1

    if not port_lista.get() or not odometro_lista.get():
        mensaje_error.config(text="Falta seleccionar el puerto o el tipo de odómetro.")
        Estado = 0
        return
    if odometro_lista.get() == "Personalizado" and not input_ratio.get():
        mensaje_error.config(text="Falta seleccionar el radio especial para odómetro personalizado.")
        Estado = 0
        return

    mensaje_error.config(text="")

    hora_actual = datetime.datetime.now().strftime("%H.%M.%S.%f")
    nombre_archivo = os.path.join(current_folder, "CSVs de prueba", f"{hora_actual}.csv")

    with open(nombre_archivo, "w", newline="") as file:
        writer = csv.writer(file, delimiter=";", quotechar='"', quoting=csv.QUOTE_NONNUMERIC)
        writer.writerow(["Hora Unix","Hora Local",  "Distancia", "Ticks", "GIROSCOPIO", "ACELEROMETRO", "MAGNETROMETRO"])

    if port_lista.get() and odometro_lista.get():
        if selec_imu.get():
            puertoSerial = serial.Serial(port_lista2.get(), 115200, timeout=2)
            puertoSerial.write("iniciar".encode())
            print("Conexión con IMU establecida")

            # Inicializa la conexión serial para el software sonar
            puertoSerial_b = serial.Serial("COM29", 115200, timeout=2)
            puertoSerial_b.write("iniciar".encode())
            print("Conexión con software sonar establecida")

        puertoSerial_c = serial.Serial(port_lista.get(), 115200, timeout=0.2)
        print(f"Conexión establecida con puerto {port_lista.get()}")

        port_lista.config(state="disabled")
        port_lista2.config(state="disabled")
        odometro_lista.config(state="disabled")
        input_reset.config(state="normal")
        input_ratio.config(state="disabled")
        b_conectar.config(state="disabled")
        b_reset.config(state="normal")
        b_desconectar.config(state="normal")
        check.config(state="disabled")

        if odometro_lista.get() == "Guia de cable":
            encoder_type = 1
            encoder_ratio = 0
            envio_encoder = True
        elif odometro_lista.get() == "Carrete":
            encoder_type = 2
            encoder_ratio = 0
            envio_encoder = True
        elif odometro_lista.get() == "Personalizado":
            encoder_type = 3
            try:
                # Intentar convertir el valor ingresado a float
                encoder_ratio = float(input_ratio.get())  # Si es válido, se guarda en begin_reset
                envio_encoder = True
            except ValueError:
                # Si ocurre un error al intentar convertir a float, muestra un mensaje de error
                mensaje_error.config(text="Error: El valor ingresado no es válido. Ingrese un número válido. Ejemplo: 0.05. Use punto para los decimales")


        if envio_encoder:
            if envio_encoder_listo: 
                mensaje = f"RUN"
                #print(mensaje)
                puertoSerial_c.write(mensaje.encode('utf-8'))
                envio_encoder = False
            else:# solo envía encoder una vez. Si se quiere cambiar de encoder se debe cerrar la aplicación
                mensaje = f"RUN,{encoder_type},{encoder_ratio}"
                #print(mensaje)
                puertoSerial_c.write(mensaje.encode('utf-8'))
                envio_encoder = False
                envio_encoder_listo = True
    
    else:

        Estado = 0

    Ti = ""
    pitch = "+00.0"
    roll = "+000.0"
    Distancia = "000.01"
    acelerometro = [0, 0, 0, 0, 0, 0]
    magnetometro = [0, 0, 0, 0, 0, 0]
    giroscopio = [0, 0, 0, 0, 0, 0]
    
    while Estado == 1:
        time.sleep(0.028)  # tiempo de muestro
        ace = " "
        giro = " "
        mag = " "
        Ti = ""
        salto_de_linea = 0
        imu = ""   
        if selec_imu.get() == 1:
            while salto_de_linea < 3: 
                if puertoSerial.in_waiting > 0:
                    lectura = puertoSerial.readline()
                    if len(lectura) > 5:
                        imu = imu + lectura.decode('cp1252')
                        salto_de_linea = salto_de_linea + 1
                        time.sleep(0.002)  # tiempo de muestro
            puertoSerial.write("OK".encode('utf-8'))
            
            if imu != "":
                lista_IMU = imu.split('\n') 
                
                if len(lista_IMU) >= 3:
                    # se separan los datos por cada sensor segun lo indicado por el indice de la trama
                    for i in lista_IMU:
                        if len(i) > 6:
                            A = i.split(",")
                            if len(A) >= 6:
                                if A[1] == "0":
                                    giroscopio = i.split(",")
                                elif A[1] == "1":
                                    acelerometro = i.split(",") 
                                elif A[1] == "2":
                                    magnetometro = i.split(",")
                    
                    ace = str(acelerometro[1]) + ";" + str(acelerometro[3]) + ";" + str(acelerometro[4]) + ";" + str(acelerometro[5])
                    giro = str(giroscopio[1]) + ";" + str(giroscopio[3]) + ";" + str(giroscopio[4]) + ";" + str(giroscopio[5])
                    mag = str(magnetometro[1]) + ";" + str(magnetometro[3]) + ";" + str(magnetometro[4]) + ";" + str(magnetometro[5]) 

        if puertoSerial_c.in_waiting > 0:  # Verifica si hay datos disponibles
            try:
                Tics = puertoSerial_c.readline()
                print(f"Tics desde serial: {Tics}")
                if len(Tics) > 0:
                        Ti = "".join(filter(lambda x: x.isdigit() or x in ['-', '+'], str(Tics)))
                        print(f"Ti: {Ti}")
                        if Ti == "":
                            Ti_int = 0
                        else:
                            Ti_int = int(Ti)
                        diferencial = ti_ant - Ti_int
                        ti_ant = Ti_int
                        Ticks = Ticks - diferencial

                        if odometro_lista.get() == "Guia de cable":
                            Distancia = round((((Ticks * 0.0372 * 3.1416) / 1024) * 1), 2)
                        elif odometro_lista.get() == "Carrete":
                            Distancia = round((((Ticks * 0.0225 * 3.1416) / 1024) * 1.0216), 2)
                        elif odometro_lista.get() == "Personalizado":
                            Distancia = round((((Ticks * float(input_ratio.get()) * 3.1416) / 1024) * 1), 2)
                        print(f"post cálculo float: {Distancia}")
                        string_DIstancia=str(Distancia).split(".")
                        print(f"post cálculo: {string_DIstancia}")

                        if len(string_DIstancia[1])<2:
                                string_DIstancia[1]= string_DIstancia[1]+"0"
                        Distancia=string_DIstancia[0]+"."+string_DIstancia[1]
                        print(f"post ajuste por largo: {Distancia}")

                        # Ajuste de formato de distancia
                        if float(Distancia) >= 0:
                            if len(str(Distancia)) == 4:
                                Distancia = "+000" + str(Distancia)
                            if len(str(Distancia)) == 5:
                                Distancia = "+00" + str(Distancia)
                            if len(str(Distancia)) == 6:
                                Distancia = "+0" + str(Distancia)
                            if len(str(Distancia)) == 7:
                                Distancia = "+" + str(Distancia)
                        else:
                            if len(str(Distancia)) == 4:
                                Distancia = "-0000" + str(Distancia[1:])
                            if len(str(Distancia)) == 5:
                                Distancia = "-000" + str(Distancia[1:])
                            if len(str(Distancia)) == 6:
                                Distancia = "-00" + str(Distancia[1:])
                            if len(str(Distancia)) == 7:
                                Distancia = "-0" + str(Distancia[1:])
                        
                        print(f"Distancia final: {Distancia}")

            except UnicodeDecodeError:
                print("Error de decodificación, ignorando los datos corruptos.")
                continue 
        
        dis.set(Distancia)
        
              
    
        formato = f"$PITCH{pitch},ROLL{roll},DIST{Distancia}\r\n"
        
        # Guardar datos en archivo CSV
        with open(nombre_archivo, 'a', newline='') as archivo:
            escritor = csv.writer(archivo, delimiter=';', quotechar='"', quoting=csv.QUOTE_NONNUMERIC)
            hora_actual_unix = datetime.datetime.now().timestamp()
            hora_actual_local = datetime.datetime.now()
            escritor.writerow([hora_actual_unix, hora_actual_local, Distancia, Ticks, giro, ace, mag])
        
        if selec_imu.get() == 1:
            puertoSerial_b.write(formato.encode('utf-8'))
        dis.set(Distancia)
        
        # Verificar si se necesita resetear la distancia
        if Estado_reset == 1:
            try:
                nueva_dis = "reset" + input_reset.get() + "@"
                puertoSerial_c.write(nueva_dis.encode('utf-8'))
                Estado_reset = 0
            except ValueError:
                pass

        ventana.update()

    if selec_imu == 1:
        puertoSerial.flushInput()
        puertoSerial.close()
        puertoSerial_b.close()
    if port_lista.get() and odometro_lista.get():
        puertoSerial_c.flushInput()
        puertoSerial_c.close()

def limpiar_error(event):
    mensaje_error.config(text="")  # Borra el mensaje de error cuando el usuario interactúa con un campo de entrada


# Habilitar o deshabilitar el campo "Radio especial"
def habilitar_radio(*args):
    if odometro_lista.get() == "Personalizado":
        input_ratio.config(state="normal")
    else:
        input_ratio.config(state="disabled")

# Configuración de la interfaz gráfica
ventana = tk.Tk()
ventana.title("Odometria - Maquintel")
ventana.geometry('800x250')
ventana.configure(background='dark orange')

logo = tk.PhotoImage(logo_path)
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

b_reset = tk.Button(ventana, text='Reset a (en metros): ', command=reset)
b_reset.place(x=155, y=155)

input_reset = tk.Entry(ventana, width=15)
input_reset.place(x=285, y=159)

odometro_lista = ttk.Combobox(ventana, width=10, state="readonly", values=['Guia de cable', 'Carrete', 'Personalizado'])
odometro_lista.place(x=250, y=105)

etiqueta4 = tk.Label(ventana, text='Tipo odometro', bg='white', fg='black', width=13)
etiqueta4.place(x=245, y=80)

etiqueta5 = tk.Label(ventana, text='Radio especial (en metros)', bg='white', fg='black', width=20)
etiqueta5.place(x=350, y=80)

input_ratio = tk.Entry(ventana, width=15)
input_ratio.place(x=350, y=105)

input_reset.bind("<FocusIn>", limpiar_error)
input_ratio.bind("<FocusIn>", limpiar_error)

selec_imu = tk.IntVar()
check = tk.Checkbutton(ventana, variable=selec_imu, onvalue=1, offvalue=0, bg="dark orange", activebackground="dark orange", text='IMU')
check.place(x=5, y=115)

mensaje_error = tk.Label(ventana, text="", bg="white", fg="red", font=("Helvetica", 10))
mensaje_error.place(x=5, y=190)

odometro_lista.bind("<<ComboboxSelected>>", habilitar_radio)

# Iniciar actualización de puertos
actualizar_puertos()
desconectar()
ventana.mainloop()
