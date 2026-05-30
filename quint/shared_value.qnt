module shared_value {
  type Snapshot = {
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
  }

  type Memory = {
    writerTaken: bool,
    writerCount: int,
    readerCount: int,
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
    currentCycle: int,
    failedWriterRequests: int,
    history: Set[Snapshot],
  }

  pure val MAX_READERS = 2
  pure val MAX_CYCLE = 3
  pure val VALUES = Set(1, 2)
  pure val READER_IDS = 0.to(MAX_READERS - 1)
  pure val NO_VALUE = 0

  pure def snapshot(hasValue: bool, value: int, cycle: int, sequence: int): Snapshot = {
    hasValue: hasValue,
    value: value,
    cycle: cycle,
    sequence: sequence,
  }

  pure def memorySnapshot(m: Memory): Snapshot =
    snapshot(m.hasValue, m.value, m.cycle, m.sequence)

  pure val emptySnapshot = snapshot(false, NO_VALUE, 0, 0)

  var memory: Memory
  var readerSnapshots: int -> Snapshot

  action init = all {
    memory' = {
      writerTaken: false,
      writerCount: 0,
      readerCount: 0,
      hasValue: false,
      value: NO_VALUE,
      cycle: 0,
      sequence: 0,
      currentCycle: 0,
      failedWriterRequests: 0,
      history: Set(emptySnapshot),
    },
    readerSnapshots' = READER_IDS.mapBy(_ => emptySnapshot),
  }

  action takeWriter = all {
    not(memory.writerTaken),
    memory' = {
      writerTaken: true,
      writerCount: memory.writerCount + 1,
      readerCount: memory.readerCount,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      currentCycle: memory.currentCycle,
      failedWriterRequests: memory.failedWriterRequests,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action rejectSecondWriter = all {
    memory.writerTaken,
    memory' = {
      writerTaken: memory.writerTaken,
      writerCount: memory.writerCount,
      readerCount: memory.readerCount,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      currentCycle: memory.currentCycle,
      failedWriterRequests: memory.failedWriterRequests + 1,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action newReader = all {
    memory.readerCount < MAX_READERS,
    memory' = {
      writerTaken: memory.writerTaken,
      writerCount: memory.writerCount,
      readerCount: memory.readerCount + 1,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      currentCycle: memory.currentCycle,
      failedWriterRequests: memory.failedWriterRequests,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action advanceCycle = all {
    memory.currentCycle < MAX_CYCLE,
    memory' = {
      writerTaken: memory.writerTaken,
      writerCount: memory.writerCount,
      readerCount: memory.readerCount,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      currentCycle: memory.currentCycle + 1,
      failedWriterRequests: memory.failedWriterRequests,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action write = {
    nondet newValue = VALUES.oneOf()
    val nextSnapshot = snapshot(true, newValue, memory.currentCycle, memory.sequence + 1)

    all {
      memory.writerTaken,
      memory' = {
        writerTaken: memory.writerTaken,
        writerCount: memory.writerCount,
        readerCount: memory.readerCount,
        hasValue: true,
        value: newValue,
        cycle: memory.currentCycle,
        sequence: memory.sequence + 1,
        currentCycle: memory.currentCycle,
        failedWriterRequests: memory.failedWriterRequests,
        history: memory.history.union(Set(nextSnapshot)),
      },
      readerSnapshots' = readerSnapshots,
    }
  }

  action read = {
    nondet reader = READER_IDS.oneOf()

    all {
      reader < memory.readerCount,
      memory' = memory,
      readerSnapshots' = readerSnapshots.set(reader, memorySnapshot(memory)),
    }
  }

  action step = any {
    takeWriter,
    rejectSecondWriter,
    newReader,
    advanceCycle,
    write,
    read,
  }

  val typeOk = all {
    memory.writerCount >= 0,
    memory.writerCount <= 1,
    memory.readerCount >= 0,
    memory.readerCount <= MAX_READERS,
    memory.sequence >= 0,
    memory.currentCycle >= 0,
    memory.currentCycle <= MAX_CYCLE,
    memory.failedWriterRequests >= 0,
    memory.history.forall(s => s.sequence >= 0),
    memory.history.forall(s => s.sequence <= memory.sequence),
    memory.history.forall(s => s.cycle >= 0),
    memory.history.forall(s => s.cycle <= MAX_CYCLE),
  }

  val singleWriter = all {
    memory.writerCount <= 1,
    memory.writerTaken == (memory.writerCount == 1),
  }

  val writeMetadata = all {
    memory.hasValue == (memory.sequence > 0),
    not(memory.hasValue).implies(memory.value == NO_VALUE),
    memory.hasValue.implies(memory.value.in(VALUES)),
    memory.hasValue.implies(memory.cycle <= memory.currentCycle),
    memorySnapshot(memory).in(memory.history),
  }

  val atomicReaderSnapshots =
    READER_IDS.forall(reader => readerSnapshots.get(reader).in(memory.history))
}
