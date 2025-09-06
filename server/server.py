#!/usr/bin/env python3

from flask import Flask, request, jsonify, abort

app = Flask(__name__)
PRIME_WRAPPER = 1381
BATCH_CUTOFF = 10


def get_pseudorandom(seed: int, cursor: int):
    return int(seed * cursor * 3) % PRIME_WRAPPER


@app.route("/api", methods=["GET"])
def get_next_item():
    seed = request.args.get("seed", default=None, type=int)
    cursor = request.args.get("cursor", default=None, type=int)
    batch = request.args.get("batch", default=1, type=int)

    app.logger.info(
        f"Received request with seed: {seed}, cursor: {cursor}, batch: {batch}"
    )

    if seed != 0 and not seed or not cursor:
        abort(400, f"Invalid request data, received seed={seed}, cursor={cursor}")

    if batch > BATCH_CUTOFF:
        abort(400, f"Your batch size is too big: {batch}")

    if batch <= 0:
        abort(400, f"Your batch size is too low:  {batch}")

    if cursor < 0:
        abort(400, f"Your cursor is too low: {cursor}")

    if seed < 1:
        abort(400, f"Your seed is too low: {seed}")

    # Send updated cursor & list of objects
    values = []
    for _ in range(batch):
        values.append(get_pseudorandom(seed, cursor))
        cursor += 1

    response = {"levels": values, "cursor": cursor}

    return jsonify(response)


@app.errorhandler(404)
def catch_all(e):
    return f"You're querying the wrong endpoint: {request.path}. On the bright side, you're connected!"


if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)
