#!/usr/bin/python

from __future__ import print_function
from multiprocessing import Process, Queue
from Queue import Empty
import simplejson as json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ATM_ORACLE = './atm'
BANK_ORACLE = './bank'

MITM_DIR = './'

def err( s):
	if False:
		print( s, file=sys.stderr)
	# print( s)

def arrToStr( a):
	return ' '.join(map( str, a))

protocolErrorReceipt = {"protocol":"error"}

def receiptToStr( r):
	if r == {}:
		return ""
	elif r == protocolErrorReceipt:
		return "protocol_error\n"
	else:
		return json.dumps( r, use_decimal=True)

def strToReceipt( s):
	# print( "strToReceipt: %s" % s)
	if s == "":
		return {}
	try:
		return json.loads( s.rstrip(), use_decimal=True)
	except:
		return None

def popQueue( q, default):
	try:
		 return q.get( timeout=0.75)
	except Empty:
		return default

def runMITM( args, d):
	err( "$ " + arrToStr( args))
	p = subprocess.Popen( args, cwd = d) # redirect or throw away stdout?
	
	# Register interupt.
	def handler( s, f):
		os.kill( p.pid, signal.SIGINT)
	
		while True:
			# err( p.poll())
			if p.poll() != None:
				break

		os.kill( os.getpid(), signal.SIGTERM)
	signal.signal(signal.SIGINT, handler)

	while True:
		# err( p.poll())
		if p.poll() != None:
			break

def runBank( args, d, q):
	err( "$ " + arrToStr( args))
	#bankProc = subprocess.Popen( args, stdout=subprocess.PIPE, cwd = d, preexec_fn=os.setsid)
	bankProc = subprocess.Popen( args, stdout=subprocess.PIPE, cwd = d)

	# Register interupt.
	def handler( s, f):
		# _ = bankProc.communicate()
		# bankProc.terminate()
		os.kill( bankProc.pid, signal.SIGINT)
		os.kill( bankProc.pid, signal.SIGTERM)
		# err("here1")
		while True:
			# err( bankProc.poll())
			if bankProc.poll() != None:
				break
		# exit(0)
		os.kill( os.getpid(), signal.SIGTERM)
	signal.signal(signal.SIGINT, handler)

	# for line in iter(bankProc.stdout.readline,''):
	# 	q.put( line)
	while True:
		line = bankProc.stdout.readline()
		# err( "bank: %s" % line)
		q.put( line)
		# err("here: %s" % line)
		if line == '' and bankProc.poll() != None:
			break

	# err("here2")
	handler( 0, 0)

def exitFailed( p1, p2):
	terminateFromMain( p1)
	terminateFromMain( p2)

	exit(1)

def testFailed( p, e):
	# if p != None:
	# 	err(p.pid)
	# 	try:
	# 		os.killpg( p.pid, signal.SIGINT)
	# 	except Exception as s:
	# 		# err("kill may have failed...")
	# 		err(s)

	# 	try:
	# 		os.killpg( p.pid, signal.SIGTERM)
	# 	except Exception as s:
	# 		# err("kill may have failed again...")
	# 		err( s)
	terminateFromMain( p)

	print( e)
	exit(1)

def terminateFromMain( p):
	if p != None:
		os.kill( p.pid, signal.SIGINT)
		# os.kill( p.pid, signal.SIGTERM)

def runTest( fileName):
	try:
		testFile = open(fileName, 'r')
	except IOError:
		return testFailed( None, "Could not open file: %s" % fileName)

	pretest = testFile.read()
	testFile.close()
	
	pretest = json.loads( pretest, use_decimal=True)

	port = '3000'
	host = '127.0.0.1'

	# TODO: Check if there is a MITM.

	# Create temp directories.
	atmDir = tempfile.mkdtemp()
	atmLoc = atmDir + "/atm"
	bankDir = tempfile.mkdtemp()
	bankLoc = bankDir + "/bank"

	# Move oracles to tmp directories. 
	shutil.copy( ATM_ORACLE, atmLoc)
	shutil.copy( BANK_ORACLE, bankLoc)

	# Run bank in background.
	bankMsgQueue = Queue()
	bankP = Process( target=runBank, args=([ bankLoc, "-p", port], bankDir, bankMsgQueue,))
	bankP.start()

	# Wait for complete.
	msg = bankMsgQueue.get()
	if msg != 'created\n':
		return testFailed( bankP, "Bank did not print 'created', got: %s" % msg)

	# Run MITM if there is one.
	mitmP = None
	mitmDir = None
	if 'mitm' in pretest.keys():
		mitm = pretest['mitm']
		port = '4000'

		# Create temp directories.
		mitmDir = tempfile.mkdtemp()
		mitmLoc = mitmDir + '/' + mitm

		# Copy mitm to tmp directories.
		shutil.copy( MITM_DIR + '/' + mitm, mitmLoc)

		# Run MITM
		mitmP = Process( target = runMITM, args=([mitmLoc,"-p", port], mitmDir,))
		mitmP.start()

	time.sleep(1)

	# Copy over auth file.
	shutil.copy( bankDir + "/bank.auth", atmDir + "/bank.auth")

	# err(pretest)

	# Run each input.
	for i in pretest['inputs']:
		# Run input on atm.
		args = i['input']['input']
		args = args = map( lambda a: a.replace("%PORT%", port).replace("%IP%", host), args)
		args.insert( 0, atmLoc)
		err( "$ " + arrToStr(args))
		p = subprocess.Popen( args, stdout=subprocess.PIPE, cwd = atmDir)
		atmOut, atmErr = p.communicate()
		atmExit = p.returncode

		expReceipt = i['output']['output']
		# expReceipt = json.loads( expOut, use_decimal=True)
		expExit = i['output']['exit']

		# Check exit code.
		if atmExit != expExit:
			print( "$ " + arrToStr(args))
			print( "got exit: %s" % str(atmExit))
			print( "expected exit: %s" % str(expExit))
			exitFailed( bankP, mitmP)

		if atmOut == "":
			atmReceipt = {}
		else:
			try:
				atmReceipt = json.loads( atmOut.rstrip(), use_decimal=True)
			except:
				atmReceipt = None
		
		# Check atm output.
		if atmReceipt != expReceipt:
			print( "$ " + arrToStr(args))
			print( "got: %s" % atmOut)
			print( "expected: %s" % receiptToStr( expReceipt))
			exitFailed( bankP, mitmP)

		expReceipt = expReceipt if expExit != 63 else protocolErrorReceipt

		# Check bank output.
		bankOut = popQueue( bankMsgQueue, "")
		bankReceipt = None
		if bankOut == "protocol_error\n":
			bankReceipt = protocolErrorReceipt
		else:
			bankReceipt = strToReceipt( bankOut)
		# print( "bankOut: %s" % bankOut)
		# print( "bankReceipt: %s" % str(bankReceipt))
		# print( "expReceipt: %s" % str(expReceipt))
		if bankReceipt != expReceipt:
			print( "$ " + arrToStr(args))
		 	print( "got bank: %s" % bankOut)
		 	print( "expected bank: %s" % receiptToStr( expReceipt))
		 	exitFailed( bankP, mitmP)
			

	time.sleep(1)
		
	# Kill bank.
	# err("hereA")
	terminateFromMain( bankP)
	terminateFromMain( mitmP)
	#os.kill( bankP.pid, signal.SIGINT)
	# err("hereB")
	# bankP.terminate()

	# Clear tmp directory contents. 
	clearDir( atmDir)
	clearDir( bankDir)
	if mitmDir != None:
		clearDir( mitmDir)


	return True


def clearDir( dir):
	for f in os.listdir( dir):
		os.remove( dir + '/' + f)

# Main
def main():
	if len( sys.argv) < 2:
		print("usage: run_test <test.json>")
		exit(1)
	
	testFileName = sys.argv[1]
	
	# Run test.
	msg = "test passed: " if runTest( testFileName) else "test failed: "
	print( msg + testFileName)

main()
