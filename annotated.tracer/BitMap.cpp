
inline void BitMap::Init(unsigned int size, unsigned int rows) {
	sz = size;
	rw = rows;
	ct = (sz + 0x1f) >> 5;
	data.resize(ct * rw, 0);

	refCount = 1;

	printf("Create [%d,%d,%d]", sz, varCount, rw);
	Print();
	printf("\n");

	instCount++;
}

static BitMap *BitMap::Union(const BitMap &b1, const BitMap &b2) {
	if ((b1.sz != b2.sz) || (b1.rw != b1.rw)) {
		return nullptr;
	}

	BitMap *ret = new BitMap(b1.sz, b1.rw);

	for (unsigned int i = 0; i < ret->rw; ++i) {
		for (unsigned int j = 0; j < ret->ct; ++j) {
			ret->data[i * ret->ct + j] = b1.data[i * ret->ct + j] | b2.data[i * ret->ct + j];
		}
	}

	printf("Union ");
	b1.Print();
	printf(", ");
	b2.Print();
	printf("=> ");
	ret->Print();
	printf("\n");

	return ret;
}

void BitMap::Union(const BitMap &rhs) {
	if (((sz != rhs.sz) || (rw != rhs.rw)) && (rw != 1)) {
		//__asm int 3;
		DEBUG_BREAK;
	}

	printf("Selfunion ");
	Print();
	printf(", ");
	rhs.Print();
	printf("=> ");

	isZero &= rhs.isZero;
	if (rw == 1) {
		for (unsigned int i = 0; i < rhs.rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[j] |= rhs.data[i * ct + j];
			}
		}
	} else {
		for (unsigned int i = 0; i < rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[i * ct + j] |= rhs.data[i * ct + j];
			}
		}
	}

	Print();
	printf("\n");
}

BitMap::BitMap(unsigned int size, unsigned int rows) {
	Init(size, rows);
	isZero = true;
}

BitMap::BitMap(const BitMap &b1, const BitMap &b2) {
	if (b1.sz != b2.sz) {
		//__asm int 3;
		DEBUG_BREAK;
	}

	Init(b1.sz, b1.rw + b2.rw);
	isZero = b1.isZero & b2.isZero;

	for (unsigned int i = 0; i < b1.rw; ++i) {
		for (unsigned int j = 0; j < ct; ++j) {
			data[i * ct + j] = b1.data[i * ct + j];
		}
	}

	for (unsigned int i = 0; i < b2.rw; ++i) {
		for (unsigned int j = 0; j < ct; ++j) {
			data[(b1.rw + i) * ct + j] = b2.data[i * ct + j];
		}
	}
}

BitMap::BitMap(const BitMap &o, unsigned int c) {
	Init(o.sz, c);

	for (unsigned int j = 0; j < ct; ++j) {
		unsigned int r = 0;
		for (unsigned int i = 0; i < o.rw; ++i) {
			r |= o.data[i * ct + j];
		}

		for (unsigned int i = 0; i < rw; ++i) {
			data[i * ct + j] = r;
		}
	}
}

BitMap::BitMap(const BitMap &o, unsigned int s, unsigned int c) {
	if (o.sz > 4) {
		DEBUG_BREAK;
	}
	Init(o.sz, c);

	if (o.rw != 1) {
		for (unsigned int i = 0; i < rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[i * ct + j] = o.data[(i + s) * ct + j];
			}
		}
	} else {
		for (unsigned int i = 0; i < rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[i * ct + j] = o.data[j];
			}
		}
	}
}

void BitMap::SetBit(unsigned int p) {
	unsigned int m = 1 << (p & 0x1F);
	for (unsigned int i = 0; i < rw; ++i) {
		data[i * ct + (p >> 5)] |= m;
	}

	isZero = false;
}

bool BitMap::GetBit(unsigned int p) const {
	unsigned int m = 1 << (p & 0x1F);
	unsigned int r = 0;

	for (unsigned int i = 0; i < rw; ++i) {
		r |= data[i * ct + (p >> 5)];
	}

	return 0 != (r & m);
}

bool BitMap::IsZero() const {
	return isZero;
}

void BitMap::Print() const {
	printf("<%08x> ", (unsigned int)this);
	for (unsigned int i = 0; i < rw; ++i) {
		for (unsigned int j = 0; j < ct; ++j) {
			printf("%08x ", data[i * ct + j]);
		}
		printf("| ");
	}
}

void BitMap::AddRef() {
	printf("AddRef <%08x>\n", (unsigned int )this);
	refCount++;
}

void BitMap::DelRef() {
	printf("DelRef <%08x>\n", (unsigned int)this);
	refCount--;
	if (0 == refCount) {
		printf("Delete ");
		Print();
		printf("\n");
		delete this;
	}
}
