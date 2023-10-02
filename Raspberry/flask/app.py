# Popola i dati della pagina web
from flask import Flask, request, render_template

app = Flask(__name__, template_folder='templates')

temperatura = None
pressione = None
velocità_ventola= None
@app.route('/ricevi-dati', methods=['POST'])
def ricevi_dati():
    global temperatura, pressione, velocità_ventola
    dati = request.get_json()

    temperatura = dati.get("temperatura")
    pressione = dati.get("pressione")
    velocità_ventola = dati.get("velocità_ventola")
    return 'Dati ricevuti con successo'

@app.route('/visualizza-dati', methods=['GET'])
def visualizza_dati():
    global temperatura, pressione, velocità_ventola
    dati = {"temperatura": temperatura, "pressione": pressione, "velocità_ventola": velocità_ventola}
    return dati

@app.route('/pagina', methods=['GET'])
def pagina():
    return render_template('index.html', temperatura=temperatura, pressione=pressione, velocità_ventola= velocità_ventola)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

