from __future__ import print_function
import sys,os
import struct
import StringIO
from PIL import Image
from struct import pack, unpack

def encode(line):
    count = 0
    lst = []
    repeat = -1
    run = []
    total = len(line) - 1
    for index, current in enumerate(line[:-1]):
        if current != line[index + 1]:
            run.append(current)
            count += 1
            if repeat == 1:
                entry = (count+128,run)
                lst.append(entry)
                count = 0
                run = []
                repeat = -1
                if index == total - 1:
                    run = [line[index + 1]]
                    entry = (1,run)
                    lst.append(entry)
            else:
                repeat = 0

                if count == 128:
                    entry = (128,run)
                    lst.append(entry)
                    count = 0
                    run = []
                    repeat = -1
                if index == total - 1:
                    run.append(line[index + 1])
                    entry = (count+1,run)
                    lst.append(entry)
        else:
            if repeat == 0:
                entry = (count,run)
                lst.append(entry)
                count = 0
                run = []
                repeat = -1
                if index == total - 1:
                    run.append( line[index + 1])
                    run.append( line[index + 1])
                    entry = (2+128,run)
                    lst.append(entry)
                    break
            run.append(current)
            repeat = 1
            count += 1
            if count == 128:
                entry = (256,run)
                lst.append(entry)
                count = 0
                run = []
                repeat = -1
            if index == total - 1:
                if count == 0:
                    run = [line[index + 1]]
                    entry = (1,run)
                    lst.append(entry)
                else:
                    run.append(current)
                    entry = (count+1+128,run)
                    lst.append(entry)
    return lst


def encodeRLE24(img):
    width, height = img.size
    output = StringIO.StringIO()
    bytesProcessedInBody = 0
    #global bodyLength
    #global payloadLimit

    for h in range(height):
        line = []
        result=[]
        for w in range(width):
            (r, g, b) = img.getpixel((w,h))
            line.append((r << 16)+(g << 8) + b)
        result = encode(line)
        for count, pixel in result:
            output.write(struct.pack("B", count-1))
            bytesProcessedInBody += 1
            if count > 128:
                output.write(struct.pack("B", (pixel[0]) & 0xFF))
                output.write(struct.pack("B", ((pixel[0]) >> 8) & 0xFF))
                output.write(struct.pack("B", ((pixel[0]) >> 16) & 0xFF))
                bytesProcessedInBody += 3
            else:
                for item in pixel:
                    output.write(struct.pack("B", (item) & 0xFF))
                    output.write(struct.pack("B", (item >> 8) & 0xFF))
                    output.write(struct.pack("B", (item >> 16) & 0xFF))
                    bytesProcessedInBody += 3

    ##The size of frame buffer allocated for OneTouch Idol 3 is just 1MB, in which first 512 bytes is for Header, so remaining payload limit: 1023.5kB (1048064 Bytes)
    # assert bytesProcessedInBody <= payloadLimit, "\n\n\nPICTURE IS LARGE IN SIZE..\nQuitting..."

    bodyLength = len(output.getvalue())

    while bytesProcessedInBody < 1000:
        output.write(struct.pack("B", 0x00))
        bytesProcessedInBody += 1
    content = output.getvalue()
    output.close()
    return content

def decodeRLE24(rle, size):
	bgcolor = (0x00, 0x00, 0x00)
	width, height = size
	img = Image.new("RGB", size, bgcolor)
	pixels = img.load()
	pos = 0
	x = 0
	y = 0
	
	hw = 0
	while hw < height * width:
		count = unpack("B", rle[pos : pos + 1])[0]
		pos += 1
		repeat_run = count & 0x80
		count = (count & 0x7f) + 1
		
		for i in range(count):
			b, g, r = unpack("BBB", rle[pos : pos + 3])
			pixels[x, y] = r, g, b
			hw += 1
			x += 1

			if not repeat_run:
				pos += 3

		if repeat_run:
			pos += 3
			
		if x == width + 0:
			y += 1
			x = 0

	return img


if __name__ == "__main__":
        #infile = "logo.png"
	#img = Image.open(infile)
	# color = (0, 0, 0)
	# background = Image.new("RGB", img.size, color)
	# img.load()
	# background.paste(img, mask=img.split()[3]) # alpha channel
	# r, g, b = background.split()
	# data = Image.merge("RGB",(b,g,r)).tobytes()
	# data = encodeRLE24(background)

	#color = (0, 0, 0)	
	#data = Image.new("RGB", [1080,1920], color)

	file = open("image.rle", "rb")
	data = file.read()
	file.close()

	decoded = decodeRLE24(data,[1080,1920])
	r, g, b = decoded.split()
	#r, g, b = data.split()
	data = Image.merge("RGB",(r,g,b)).tobytes()
	file = open("image.raw", "wb")
	file.write(data)
	file.close()
	#print(decoded)