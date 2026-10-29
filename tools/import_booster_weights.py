"""Regenerate the compiled default booster-sheet weights from the supplied 2ED export."""
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((root / 'data/source/2ED.json').read_text(encoding='utf-8'))['data']
sheets = data['booster']['default']['sheets']
rows = ['// Generated from data/source/2ED.json default booster sheets.\n']
for card in data['cards']:
    weights = ','.join(str(sheets[name]['cards'].get(card['uuid'], 0)) for name in ['common', 'uncommon', 'rare'])
    rows.append('{"' + card['uuid'] + '", {' + weights + '}},\n')
(root / 'src/club/BoosterWeights.inc').write_text(''.join(rows), encoding='utf-8')
