import subprocess

max14819_registers = [
   {'regnr': 0x00, 'name': 'TxRxDataA',  'bits': ['D7A', 'D6A', 'D5A', 'D4A', 'D3A', 'D2A', 'D1A', 'D0A']},
   {'regnr': 0x01, 'name': 'TxRxDataB',  'bits': ['D7B', 'D6B', 'D5B', 'D4B', 'D3B', 'D2B', 'D1B', 'D0B']},
   {'regnr': 0x02, 'name': 'Interupt',   'bits': ['StatusErr', 'WURQErr', 'TxErrorB', 'TxErrorA', 'RxErrorB', 'RxErrorA', 'RxDataRdyB', 'RxDataRdyA']},
   {'regnr': 0x03, 'name': 'InteruptEn', 'bits': ['StatusIntEn', 'WURQErrEn', 'TxErrIntEnB', 'TxErrIntEnA', 'RxErrIntEnB', 'RxErrIntEnA', 'RDaRdyIntEnB', 'RDaRdyIntEnA']},
   {'regnr': 0x04, 'name': 'RxFIFOLvlA', 'bits': ['FifoLvl7A', 'FifoLvl6A', 'FifoLvl5A', 'FifoLvl4A', 'FifoLvl3A', 'FifoLvl2A', 'FifoLvl1A', 'FifoLvl0A']},
   {'regnr': 0x05, 'name': 'RxFIFOLvlB', 'bits': ['FifoLvl7B', 'FifoLvl6B', 'FifoLvl5B', 'FifoLvl4B', 'FifoLvl3B', 'FifoLvl2B', 'FifoLvl1B', 'FifoLvl0B']},
   {'regnr': 0x06, 'name': 'CQCntrolA',  'bits': ['ComRt1A', 'ComRt0A', 'EstComA', 'WuPulsA', 'TxFifoRstA', 'RxFifoRstA', 'CyclTmrEnA', 'CQSendA']},
   {'regnr': 0x07, 'name': 'CQCntrolB',  'bits': ['ComRt1B', 'ComRt0B', 'EstComB', 'WuPulsB', 'TxFifoRstB', 'RxFifoRstB', 'CyclTmrEnB', 'CQSendB']},
   {'regnr': 0x08, 'name': 'CQErrorA',   'bits': ['TransmErrA', 'TCyclErrA', 'TChksmErA', 'TSizeErrA', 'RChksmErA', 'RSizeErrA', 'FrameErrA', 'ParityErrA']},
   {'regnr': 0x09, 'name': 'CQErrorB',   'bits': ['TransmErrB', 'TCyclErrB', 'TChksmErB', 'TSizeErrB', 'RChksmErB', 'RSizeErrB', 'FrameErrB', 'ParityErrB']},
   {'regnr': 0x0A, 'name': 'MesgCntlA',  'bits': ['TxErDestroyA', 'SPIChksA', 'InsChksA', 'TSizeEnA', 'TxKeepMsgA', 'RChksEnA', 'RMessgRdyEnA', 'InvCQA']},
   {'regnr': 0x0B, 'name': 'MesgCntlB',  'bits': ['TxErDestroyB', 'SPIChksB', 'InsChksB', 'TSizeEnB', 'TxKeepMsgB', 'RChksEnB', 'RMessgRdyEnB', 'InvCQB']},
   {'regnr': 0x0C, 'name': 'ChanStatA',  'bits': ['RstA', 'FramerEnA', 'L+CLimErrA', 'UVL+ErrA', 'CQLimErrA', 'L+CLimA', 'UVL+A', 'CQLimA']},
   {'regnr': 0x0D, 'name': 'ChanStatB',  'bits': ['RstB', 'FramerEnB', 'L+CLimErrB', 'UVL+ErrB', 'CQLimErrB', 'L+CLimB', 'UVL+B', 'CQLimB']},
   {'regnr': 0x0E, 'name': 'LEDCntrl',   'bits': ['LEDEn2B', 'RxErrEnB', 'LEDEn1B', 'RxRdyEnB', 'LEDEn2A', 'RxErrEnA', 'LEDEn1A', 'RxRdyEnA']},
   {'regnr': 0x0F, 'name': 'Trigger',    'bits': ['-', '-', '-', '-', 'Trigger3', 'Trigger2', 'Trigger1', 'Trigger0']},
   {'regnr': 0x10, 'name': 'CQConfgA',   'bits': ['IEC3ThA', 'SorceSinkA', 'SinkSel1A', 'SinkSel0A', 'NPNA', 'PushPulA', 'DrvrDisA', 'RFilterEnA']},
   {'regnr': 0x11, 'name': 'CQConfgB',   'bits': ['IEC3ThB', 'SorceSinkB', 'SinkSel1B', 'SinkSel0B', 'NPNB', 'PushPulB', 'DrvrDisB', 'RFilterEnB']},
   {'regnr': 0x12, 'name': 'CyclTimeA',  'bits': ['TCycBsA1', 'TCycBsA0', 'TCycMA5', 'TCycMA4', 'TCycMA3', 'TCycMA2', 'TCycMA1', 'TCycMA0']},
   {'regnr': 0x13, 'name': 'CyclTimeB',  'bits': ['TCycBsB1', 'TCycBsB0', 'TCycMB5', 'TCycMB4', 'TCycMB3', 'TCycMB2', 'TCycMB1', 'TCycMB0']},
   {'regnr': 0x14, 'name': 'DevicDelyA', 'bits': ['DelayErrA', 'BDelay1A', 'BDelay0A', 'DDelay3A', 'DDelay2A', 'DDelay1A', 'DDelay0A', 'RspnsTmrEnA']},
   {'regnr': 0x15, 'name': 'DevicDelyB', 'bits': ['DelayErrB', 'BDelay1B', 'BDelay0B', 'DDelay3B', 'DDelay2B', 'DDelay1B', 'DDelay0B', 'RspnsTmrEnB']},
   {'regnr': 0x16, 'name': 'TrigAssgnA', 'bits': ['Trig3A', 'Trig2A', 'Trig1A', 'Trig0A', '-', '-', '-', 'TrigEnA']},
   {'regnr': 0x17, 'name': 'TrigAssgnB', 'bits': ['Trig3B', 'Trig2B', 'Trig1B', 'Trig0B', '-', '-', '-', 'TrigEnB']},
   {'regnr': 0x18, 'name': 'L+ConfigA',  'bits': ['L+RT1A', 'L+RT0A', 'L+DynBLA', 'L+BL1A', 'L+BL0A', 'L+CL2xA', 'L+CLimDisA', 'L+EnA']},
   {'regnr': 0x19, 'name': 'L+ConfigB',  'bits': ['L+RT1B', 'L+RT0B', 'L+DynBLB', 'L+BL1B', 'L+BL0B', 'L+CL2xB', 'L+CLimDisB', 'L+EnB']},
   {'regnr': 0x1A, 'name': 'IoStatCfgA', 'bits': ['DiLevelA', 'CQLevelA', 'TxEnA', 'TxA', 'DiFilterEnA', 'DiIEC3ThA', 'DiCSorceA', 'DiCSinkA']},
   {'regnr': 0x1B, 'name': 'IoStatCfgB', 'bits': ['DiLevelB', 'CQLevelB', 'TxEnB', 'TxB', 'DiFilterEnB', 'DiIEC3ThB', 'DiCSorceB', 'DiCSinkB']},
   {'regnr': 0x1C, 'name': 'DrvrCurLim', 'bits': ['CL1', 'CL0', 'CLDis', 'CLBL1', 'CLBL0', 'TAr1', 'TAr0', 'ArEn']},
   {'regnr': 0x1D, 'name': 'Clock',      'bits': ['VCCWEn', 'TxTxenDis', '-', 'ClkOEn', 'ClkDiv1', 'ClkDiv0', 'ExtClkEn', 'XtalEn']},
   {'regnr': 0x1E, 'name': 'Status',     'bits': ['ThrmShutErr', 'ThrmWarnErr', 'UVCCErr', 'VCCWarn', 'ThrmShut', 'TempWarn', 'UVCC', 'VCCWarn']},
   {'regnr': 0x1F, 'name': 'RevID',      'bits': ['-', '-', '-', '-', 'ID3', 'ID2', 'ID1', 'ID0']}]

def max14819_read_register(chip_addr, register):
   txbyte = 0x80 + (chip_addr << 5) + register
   result = subprocess.run(['./spi_test', '-D', '/dev/spidev0.0', '-s', '4000000', 
                         '-p', '\\x{0:02x}\\x00'.format(txbyte)], 
                        capture_output=True)
#   print('Args: {0}'.format(result.args))
#   print(result.stdout)
   return str(result.stdout).split('\\n')[-2].split(' ')[2:4]

def isbitset(val, bit):
   return 1 if (val & (1 << bit) > 0) else 0

def register_details(chip_addr, register):
   res = int(max14819_read_register(0, register['regnr'])[1], 16)
   print('{0}: 0x{1:02x}. '.format(register['name'], res), end='')
   for bitpos, bitname in enumerate(register['bits']):
      if bitpos > 0:
         print(', ', end='')
      print('{0}: {1}'.format(bitname, isbitset(res, 7 - bitpos)), end='')
   print('')

def dump_registers():
   for reg in max14819_registers:
      res = max14819_read_register(0, reg['regnr'])
      print('{0}: {1}'.format(reg['name'].rjust(10), res[1]))

def max1819_set_register(chip_addr, register, value):
   txbyte = (chip_addr << 5) + register
   result = subprocess.run(['./spi_test', '-D', '/dev/spidev0.0', '-s', '4000000',
                         '-p', '\\x{0:02x}\\x{1:02x}'.format(txbyte, value)])


dump_registers()
#register_details(0, max14819_registers[0x0E])
