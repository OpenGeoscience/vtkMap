#Convert input image file to C array of bytes that can be
#used to initialize vtkImageData.
#Requires PIL / Pillow module.

from __future__ import print_function

import os
import sys
from PIL import Image

if __name__ == '__main__':
  if len(sys.argv) < 3:
    print()
    print('Create .h file with data from image')
    print('Usage: python  inputfile  outputfile  [variable_name]')
    print()
    sys.exit(-1)

#Load input image(file)
  image_filename = sys.argv[1]
  input_image = Image.open(image_filename)
#Flip image to match VTK orientation
  im = input_image.transpose(Image.FLIP_TOP_BOTTOM)
  print('mode', im.mode, ', bands', im.getbands())
  format_list = ['%3d'] * len(im.getbands())
  format_string = ', '.join(format_list)
  print('format_string', format_string)

  output_filename = sys.argv[2]

  var_name = None
  if len(sys.argv) > 3:
    var_name = sys.argv[3]
  else:
#Generate from output file name
    basename = os.path.basename(output_filename)
    root, ext = os.path.splitext(basename)
    var_name = root

#Initialize output file
  with open(output_filename, 'w') as out:
    out.write('const unsigned char %s[] = {\n' % var_name)

    iter = im.getdata()
    last_index = len(iter) - 1
    for i, pixel in enumerate(iter):
      out.write('  ')
      out.write(format_string % pixel)
      if i < last_index:
        out.write(',')
      out.write('\n')
    out.write('};\n')
    print('Wrote', output_filename)
