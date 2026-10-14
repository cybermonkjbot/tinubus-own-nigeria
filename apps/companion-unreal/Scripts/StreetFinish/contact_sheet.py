"""Arrange unmodified native screenshots with street labels for visual review."""
from pathlib import Path
import sys

from PIL import Image, ImageDraw

folder=Path(sys.argv[1])
prefix=sys.argv[2]
files=sorted(folder.glob(prefix+'-*.png'))
files=[p for p in files if 'contact-sheet' not in p.name]
width,height=352,265
canvas=Image.new('RGB',(width*4,height*((len(files)+3)//4)),(25,25,25))
draw=ImageDraw.Draw(canvas)
for i,path in enumerate(files):
    image=Image.open(path).convert('RGB')
    image.thumbnail((352,243))
    x,y=(i%4)*width,(i//4)*height
    canvas.paste(image,(x,y+22))
    draw.text((x+6,y+4),path.stem.removeprefix(prefix+'-'),fill='white')
destination=folder/(prefix+'-contact-sheet.png')
canvas.save(destination)
print(destination)
