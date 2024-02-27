import tkinter as tk
import socket
import threading

class CheckersClient:
    def __init__(self, master, ip_address, port):
        master.title("Warcaby")

        self.master = master
        self.master.title("Warcaby")
        self.canvas = tk.Canvas(self.master, width=400, height=400)
        self.canvas.pack()

        #basic setup
        self.player_number = 1
        self.Board = []
        board_str = "-b-b-b-bb-b-b-b--b-b-b-bo-o-o-o--o-o-o-oa-a-a-a--a-a-a-aa-a-a-a-"
        self.update_board_from_string(board_str)
        self.draw_board()
        self.place_pawns()

        self.canvas.bind('<Button-1>', self.on_board_click)
        self.selected_pawn = None
        self.selected_pawn_id = None

        self.text_area = tk.Text(master, height=10, width=50)
        self.text_area.pack()

        self.connect_to_server(ip_address, port)

    def connect_to_server(self, ip_address, port):
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((ip_address, port))
            self.receive_thread = threading.Thread(target=self.receive_moves)
            self.receive_thread.start()
            self.text_area.insert(tk.END, f"Połączono z serwerem. Pozekaj na drugiego gracza.\n")
        except Exception as e:
            warning_message = f"Połączenie nieudane. Zrestartuj grę.\n"
            self.text_area.insert(tk.END, warning_message)

    def receive_moves(self):
        while True:
            try:
                msg = self.client_socket.recv(1024).decode("utf-8")
                if len(msg) == 64 and msg[0] in "-":
                    self.update_board_from_string(msg)
                    self.redraw_board()
                elif "Welcome Player 1" in msg:
                    self.player_number = 1
                    self.text_area.insert(tk.END, "You are Player 1\n")
                elif "Welcome Player 2" in msg:
                    self.player_number = 2
                    self.redraw_board()
                    self.text_area.insert(tk.END, "You are Player 2\n")
                else:
                    self.text_area.insert(tk.END, msg + "\n")
            except OSError:
                break

    def send_move(self):
        move = self.entry.get()
        self.client_socket.send(bytes(move, "utf-8"))
        self.entry.delete(0, tk.END)

    def make_move(self, src, dest):
        if self.player_number == 1:
            #player 1 perspective
            move_str = "{}{} {}{}".format(chr(src[1] + ord('a')), 8 - src[0], chr(dest[1] + ord('a')), 8 - dest[0])
        else:
            #player 2 perspective is inverted
            src_row, src_col = 7 - src[0], 7 - src[1]
            dest_row, dest_col = 7 - dest[0], 7 - dest[1]
            move_str = "{}{} {}{}".format(chr(src_col + ord('a')), src_row + 1, chr(dest_col + ord('a')), dest_row + 1)

        self.client_socket.sendall(move_str.encode('utf-8'))

    def on_board_click(self, event):
        col = event.x // 50
        row = event.y // 50

        #player 2 perspective
        logical_row, logical_col = (7 - row, 7 - col) if self.player_number == 2 else (row, col)

        if self.selected_pawn is not None: #look for destination click
            src = self.selected_pawn
            dest = (logical_row, logical_col)
            self.make_move(src, dest)
            self.deselect_pawn()
        else: #select first piece
            piece_id = self.get_piece_at(row, col)
            if piece_id:
                self.selected_pawn = (logical_row, logical_col) #store logical because inverted values
                self.selected_pawn_id = piece_id
                self.highlight_pawn(piece_id)

    def get_piece_at(self, row, col): #find a piece
        piece_tag = f"pawn_{row}_{col}"
        items = self.canvas.find_withtag(piece_tag)
        return items[0] if items else None

    def highlight_pawn(self, pawn_id):
        self.canvas.itemconfig(pawn_id, outline="red", width=2)

    def deselect_pawn(self):
        if self.selected_pawn_id:
            self.canvas.itemconfig(self.selected_pawn_id, outline="", width=1) #reset drawing
            self.selected_pawn = None
            self.selected_pawn_id = None

    def update_board_from_string(self, board_str): #parse received string into a board representation
        self.Board = [list(board_str[i:i + 8]) for i in range(0, 64, 8)]

    def redraw_board(self):
        self.canvas.delete("all") #clear canvas
        self.draw_board()  #draw empty board
        self.place_pawns() #redraw pawns

    def draw_board(self):
        tile_color = "saddlebrown"
        rows = range(7, -1, -1) if self.player_number == 2 else range(8) #player 2 perspective is inverted
        for row in rows:
            tile_color = "wheat" if tile_color == "saddlebrown" else "saddlebrown"
            for col in range(8):
                x1 = col * 50
                y1 = row * 50
                x2 = x1 + 50
                y2 = y1 + 50
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=tile_color, tags=f"tile{row}{col}")
                tile_color = "wheat" if tile_color == "saddlebrown" else "saddlebrown"

    def place_pawns(self):
        for row in range(8):
            for col in range(8):
                if self.Board[row][col] == "-":
                    continue
                elif self.Board[row][col] == 'a' or self.Board[row][col] == 'A': #a for player1, A is king
                    color = "white"
                elif self.Board[row][col] == 'b' or self.Board[row][col] == 'B': #b for player2, B is king
                    color = "black"
                else:
                    continue
                display_row = 7 - row if self.player_number == 2 else row
                self.draw_pawn(display_row, col, color, row)

    def draw_pawn(self, display_row, col, color, row):
        x1 = col * 50 + 12.5
        y1 = display_row * 50 + 12.5  #player 2 perspective is inverted
        x2 = x1 + 25
        y2 = y1 + 25
        pawn_tag = f"pawn_{display_row}_{col}"
        self.canvas.create_oval(x1, y1, x2, y2, fill=color, tags=(pawn_tag, "pawn"))
        if self.Board[row][col].isupper(): #look for uppercase = king
            self.canvas.create_oval(x1 + 5, y1 + 5, x2 - 5, y2 - 5, fill="gold", tags=(pawn_tag, "king"))

def show_connection_dialog():
    def on_ok():
        ip_address = ip_entry.get()
        port = port_entry.get()
        if ip_address and port:
            try:
                port = int(port)
                connection_window.destroy()
                main_window = tk.Tk()
                CheckersClient(main_window, ip_address, port)
                main_window.mainloop()
            except ValueError:
                tk.messagebox.showerror("Nieprawidłowa wartość", "Port musi być liczbą.")

    connection_window = tk.Tk() #login window
    connection_window.title("Warcaby")
    connection_window.geometry("300x150")

    tk.Label(connection_window, text="Adres IP:").pack()
    ip_entry = tk.Entry(connection_window)
    ip_entry.pack()
    default_ip_address = "192.168.0.181"
    ip_entry.insert(0, default_ip_address)

    tk.Label(connection_window, text="Port:").pack()
    port_entry = tk.Entry(connection_window)
    port_entry.pack()
    default_port = "1234"
    port_entry.insert(0, default_port)

    tk.Label(connection_window, text="", height=1).pack()
    tk.Button(connection_window, text="Graj!", command=on_ok).pack()

    connection_window.mainloop()

if __name__ == "__main__":
    show_connection_dialog()