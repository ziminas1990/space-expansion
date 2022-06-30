import asyncio
from typing import Dict
import json
import random
import secrets

import quart.flask_patch
from quart import (
    Quart,
    render_template,
    websocket,
    url_for,
    request,
    make_response
)
from flask_login import LoginManager, login_user, current_user, login_required

from expansion import types

from backend.user import User
from backend.update import Item, Update

app = Quart(__name__)
# No need to keep it the same when restarting the server
app.secret_key = secrets.token_hex()

login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'


async def stream_updates(socket, user):
    if not user.is_authenticated:
        return

    sent_updates: Dict[Item.Key, Item] = {}
    while True:
        await asyncio.sleep(0.1)
        if not user.connected():
            continue
        now = user.now
        random.seed()
        update = Update()
        update.timestamp = now
        # Collect ship updates
        for ship in user.ships:
            item_key = (types.ObjectType.SHIP, ship.name)
            item = sent_updates.setdefault(item_key, Item(*item_key))
            # Check if update is required
            if item.update(ship=ship, now=now):
                update.items.append(item)

        for asteroid in user.asteroids.values():
            assert isinstance(asteroid, types.PhysicalObject)
            item_key = (types.ObjectType.ASTEROID, asteroid.object_id)
            item = sent_updates.setdefault(item_key, Item(*item_key))
            # Check if update is required
            if item.update(asteroid=asteroid, now=now):
                update.items.append(item)

        await socket.send(json.dumps(update.to_pod()))


@login_manager.user_loader
def load_user(user_id: str):
    return User.load(user_id)


@app.route("/")
@app.route("/index")
@login_required
async def main():
    return await render_template(
        "index.html",
        connection_js=url_for('static', filename='Connection.js'),
        items_container_js=url_for('static', filename='ItemsContainer.js'),
        stage_view_js=url_for('static', filename='StageView.js'),
        scene_js=url_for('static', filename='Scene.js'),
        main_js=url_for('static', filename='main.js')
    )


@app.route('/login', methods=['GET', 'POST'])
async def login():
    if current_user.is_authenticated:
        return quart.redirect(url_for('main'))

    if request.method == "POST":
        data = await request.form
        login = data.get("login")
        user = User.load(login)

        error = await user.login(ip=data.get("ip"),
                                 login=login,
                                 password=data.get("password"))
        if not error:
            error = await user.initialize()

        if error:
            return quart.redirect(url_for('login'))
        login_user(user)
        return quart.redirect(quart.url_for('main'))
    else:
        return await quart.render_template('login.html')


@app.route('/token')
@login_required
async def token():
    return make_response(json.dumps({
        "user": current_user.user_id,
        "token": current_user.token
    }))


@app.websocket("/ws/<string:user>/<string:token>")
async def ws(user, token):
    user = User.access(user, token)
    if user is None:
        return 'Invalid token', 403
    else:
        await websocket.accept()
        await stream_updates(websocket, user)
