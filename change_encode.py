#i!/usr/bin/env python3
# -*- coding:utf-8 -*-
import os 
import sys 
import codecs 
import chardet 
  
def convert(filename,out_enc="UTF-8"): 
  try: 
    content=codecs.open(filename,'rb').read()
    source_encoding=chardet.detect(content)['encoding'] 
    print ("fileencoding:%s" % source_encoding)

    if (source_encoding == "utf-8"):
      print ("fileencoding is %s, exit." % source_encoding)
      return

    if source_encoding != None :
      content=content.decode(source_encoding, errors = 'ignore').encode(out_enc) 
      codecs.open(filename,'wb').write(content)
      #content.close()
    else :
      print("can not recgonize file encoding %s" % filename)
  except IOError as err: 
    print("I/O error:{0}".format(err)) 
  
def explore(dir): 
  print ("dir:%s" % dir)
  for root,dirs,files in os.walk(dir): 
    for file in files: 
      #if os.path.splitext(file)[1]=='.c': 
      #if os.path.splitext(file)[1]=='.h': 
      if os.path.splitext(file)[1]=='.s': 
        print ("fileName:%s" % file)
        path=os.path.join(root,file)
        print ("path:%s" % path)
        convert(path) 
  
def main(): 
  explore(os.getcwd()) 
  #filePath = input("please input dir: \n")
  #explore(filePath)

  
if __name__=="__main__": 
  main()
