module shared_value {
  type Snapshot = {
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
  }

  type Memory = {
    writerTaken: bool,
    writerSuccesses: int,
    writerFailures: int,
    readers: Set[int],
    readersThatRead: Set[int],
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
    writeFailures: int,
    history: Set[Snapshot],
  }

  pure val READER_IDS = 0.to(1)
  pure val CYCLES = 0.to(2)
  pure val VALUES = Set(1, 2)
  pure val MAX_SEQUENCE = 2
  pure val NO_VALUE = 0

  pure def snapshot(hasValue: bool, value: int, cycle: int, sequence: int): Snapshot = {
    hasValue: hasValue,
    value: value,
    cycle: cycle,
    sequence: sequence,
  }

  pure val emptySnapshot = snapshot(false, NO_VALUE, 0, 0)

  pure def memorySnapshot(m: Memory): Snapshot =
    snapshot(m.hasValue, m.value, m.cycle, m.sequence)

  var memory: Memory
  var readerSnapshots: int -> Snapshot

  action init = all {
    memory' = {
      writerTaken: false,
      writerSuccesses: 0,
      writerFailures: 0,
      readers: Set(),
      readersThatRead: Set(),
      hasValue: false,
      value: NO_VALUE,
      cycle: 0,
      sequence: 0,
      writeFailures: 0,
      history: Set(emptySnapshot),
    },
    readerSnapshots' = READER_IDS.mapBy(_ => emptySnapshot),
  }

  action takeWriter = all {
    not(memory.writerTaken),
    memory' = {
      writerTaken: true,
      writerSuccesses: memory.writerSuccesses + 1,
      writerFailures: memory.writerFailures,
      readers: memory.readers,
      readersThatRead: memory.readersThatRead,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      writeFailures: memory.writeFailures,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action rejectAdditionalWriter = all {
    memory.writerTaken,
    memory' = {
      writerTaken: memory.writerTaken,
      writerSuccesses: memory.writerSuccesses,
      writerFailures: memory.writerFailures + 1,
      readers: memory.readers,
      readersThatRead: memory.readersThatRead,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      writeFailures: memory.writeFailures,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action takeReader = {
    nondet reader = READER_IDS.oneOf()

    all {
      not(reader.in(memory.readers)),
      memory' = {
        writerTaken: memory.writerTaken,
        writerSuccesses: memory.writerSuccesses,
        writerFailures: memory.writerFailures,
        readers: memory.readers.union(Set(reader)),
        readersThatRead: memory.readersThatRead,
        hasValue: memory.hasValue,
        value: memory.value,
        cycle: memory.cycle,
        sequence: memory.sequence,
        writeFailures: memory.writeFailures,
        history: memory.history,
      },
      readerSnapshots' = readerSnapshots,
    }
  }

  action writeInTaskCycle = {
    nondet newValue = VALUES.oneOf()
    nondet taskCycle = CYCLES.oneOf()
    val nextSnapshot = snapshot(true, newValue, taskCycle, memory.sequence + 1)

    all {
      memory.writerTaken,
      memory.sequence < MAX_SEQUENCE,
      memory' = {
        writerTaken: memory.writerTaken,
        writerSuccesses: memory.writerSuccesses,
        writerFailures: memory.writerFailures,
        readers: memory.readers,
        readersThatRead: memory.readersThatRead,
        hasValue: true,
        value: newValue,
        cycle: taskCycle,
        sequence: memory.sequence + 1,
        writeFailures: memory.writeFailures,
        history: memory.history.union(Set(nextSnapshot)),
      },
      readerSnapshots' = readerSnapshots,
    }
  }

  action rejectWriteOutsideTaskCycle = all {
    memory.writerTaken,
    memory' = {
      writerTaken: memory.writerTaken,
      writerSuccesses: memory.writerSuccesses,
      writerFailures: memory.writerFailures,
      readers: memory.readers,
      readersThatRead: memory.readersThatRead,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: memory.cycle,
      sequence: memory.sequence,
      writeFailures: memory.writeFailures + 1,
      history: memory.history,
    },
    readerSnapshots' = readerSnapshots,
  }

  action read = {
    nondet reader = memory.readers.oneOf()

    all {
      memory' = {
        writerTaken: memory.writerTaken,
        writerSuccesses: memory.writerSuccesses,
        writerFailures: memory.writerFailures,
        readers: memory.readers,
        readersThatRead: memory.readersThatRead.union(Set(reader)),
        hasValue: memory.hasValue,
        value: memory.value,
        cycle: memory.cycle,
        sequence: memory.sequence,
        writeFailures: memory.writeFailures,
        history: memory.history,
      },
      readerSnapshots' = readerSnapshots.set(reader, memorySnapshot(memory)),
    }
  }

  action step = any {
    takeWriter,
    rejectAdditionalWriter,
    takeReader,
    writeInTaskCycle,
    rejectWriteOutsideTaskCycle,
    read,
  }

  val typeOk = all {
    memory.writerSuccesses >= 0,
    memory.writerSuccesses <= 1,
    memory.writerFailures >= 0,
    memory.writeFailures >= 0,
    memory.readers.subseteq(READER_IDS),
    memory.readersThatRead.subseteq(memory.readers),
    memory.sequence >= 0,
    memory.sequence <= MAX_SEQUENCE,
    memory.hasValue.implies(memory.value.in(VALUES)),
    memory.cycle.in(CYCLES),
    memory.history.forall(s => s.sequence >= 0),
    memory.history.forall(s => s.sequence <= memory.sequence),
    memory.history.forall(s => s.cycle.in(CYCLES)),
    memory.history.forall(s => s.hasValue.implies(s.value.in(VALUES))),
  }

  val writerContract = all {
    memory.writerSuccesses <= 1,
    memory.writerTaken == (memory.writerSuccesses == 1),
    memory.writerFailures > 0 implies memory.writerTaken,
  }

  val optionalValueContract = all {
    memory.hasValue == (memory.sequence > 0),
    not(memory.hasValue).implies(memorySnapshot(memory) == emptySnapshot),
  }

  val writeMetadataContract = all {
    memorySnapshot(memory).in(memory.history),
    memory.history.contains(emptySnapshot),
    memory.history.forall(s => s.hasValue == (s.sequence > 0)),
  }

  val readerSnapshotContract =
    memory.readersThatRead.forall(reader => readerSnapshots.get(reader).in(memory.history))
}
