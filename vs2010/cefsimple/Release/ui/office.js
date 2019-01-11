function _readLocalXml(path) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open("GET", path, false); // false: sync request
	xmlhttp.send(null);
	var docElem = xmlhttp.responseXML.documentElement;
	return docElem;
}

function _readLocalXml2(path) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open("GET", path, false); // false: sync request
	xmlhttp.send(null);
	return {xml:xmlhttp.responseXML, txt:xmlhttp.responseText};
}

function _partNameToPath(basePath, partName) {
	return basePath + '\\' + partName.replace(/[/]/g, '\\');
}

function Workbook(zip, destPath) {
	this._zip = zip;
	this._destPath = destPath;
	this._readySheets = false;
	this._partNames = {}; // zip part names
	this._partNames._workbookRels = 'xl/_rels/workbook.xml.rels';
	
	var FILE_NAME = '[Content_Types].xml';
	var it = zip.findItem(FILE_NAME);
	var b = zip.unzipItem(it.index, destPath + '\\' + FILE_NAME);
	var docElem = _readLocalXml(destPath + '\\' + FILE_NAME);
	var child = docElem.childNodes;
	for (var i = 0; i < child.length; ++i) {
		var ct = child[i].getAttribute('ContentType');
		var pn = child[i].getAttribute('PartName');
		if (pn && pn.charAt(0) == '/') {
			pn = pn.substring(1);
		}
		if (ct.indexOf('sheet.main+xml') > 0) {
			this._partNames._workbook = pn;
		} else if (ct.indexOf('styles+xml') > 0) {
			this._partNames._style = pn;
		} else if (ct.indexOf('sharedStrings+xml') > 0) {
			this._partNames._sharedString = pn;
		}
	}
	
	it = zip.findItem(this._partNames._workbook);
	b = zip.unzipItem(it.index, _partNameToPath(destPath, this._partNames._workbook));
	it = zip.findItem(this._partNames._workbookRels);
	b = zip.unzipItem(it.index, _partNameToPath(destPath, this._partNames._workbookRels));
	
	docElem = _readLocalXml(_partNameToPath(destPath, this._partNames._workbook));
	child = docElem.childNodes;
	var sheets = [];
	for (var i = 0; i < child.length; ++i) {
		if (child[i].nodeName == 'sheets') {
			var sc = child[i].childNodes;
			for (var j = 0; j < sc.length; ++j) {
				if (sc[j].nodeType != 1) continue;
				sheets.push({name:sc[j].getAttribute('name'), rId:sc[j].getAttribute('r:id')});
			}
			break;
		}
	}
	
	docElem = _readLocalXml(_partNameToPath(destPath, this._partNames._workbookRels));
	child = docElem.childNodes;
	var rsh = {};
	for (var i = 0; i < child.length; ++i) {
		if (child[i].nodeType != 1) continue;
		var ps = child[i].getAttribute('Target');
		if (ps.charAt(0) != '/') {
			ps = 'xl/' + ps;
		} else {
			ps = ps.substring(1);
		}
		rsh[child[i].getAttribute('Id')] = ps;
	}
	for (var i = 0; i < sheets.length; ++i) {
		sheets[i].partName = rsh[sheets[i].rId];
	}
	this._sheets = sheets; // {name:'', partName:''}
	// console.log(sheets);
}

function Sheet(wb, txt, xml) {
	this._workbook = wb;
	this._txt = txt;
	this._xml = xml;
	this._docElem = xml.documentElement;
	this._cells = [];
}
Sheet.prototype._initSheetFormatPr = function(node) {
	var a = node.getAttribute('defaultColWidth');
	if (a) this._defColWidth = a;
	a = node.getAttribute('defaultRowHeight');
	if (a) this._defRowHeight = a;
}
Sheet.prototype._initCols = function(node) {
	
}

// n = 'A23' return A is col 0
function _getColIndex(n) {
	for (var i = 0; i < n.length; ++i) {
		if (n.charAt(i) >= '0' && n.charAt(i) <= '9') {
			n = n.substring(0, i);
			break;
		}
	}
	var c = 0, jw = 1;
	for (var i = n.length - 1; i >= 0; --i) {
		c += (n.charCodeAt(i) - 65) * jw;
		jw *= 26;
	}
	return c;
}

Sheet.prototype._initSheetData = function(node) {
	var ss = this._workbook._strings;
	var child = node.childNodes;
	// console.log(node);
	for (var i = 0; i < child.length; ++i) {
		if (child[i].nodeType != 1) continue;
		// child[i] is 'row' element
		var r = child[i].getAttribute('r');
		r = parseInt(r) - 1;
		var row = [];
		var cc = child[i].childNodes;
		for (var j = 0; j < cc.length; ++j) {
			if (cc[j].nodeType != 1) continue;
			// cc[j] is 'c' element
			var attrs = cc[j].attributes;
			var cell = {};
			var cr = _getColIndex(attrs.r.value);
			if (attrs.s) cell.s = attrs.s.value;
			if (attrs.t) cell.t = attrs.t.value;
			
			row[cr] = cell;
			var cv = cc[j].childNodes;
			for (var k = 0; k < cv.length; ++k) {
				if (cv[k].nodeType != 1) continue;
				cell[cv[k].nodeName] = cv[k].textContent;
			}
			if (cell.t && cell.t == 's' && cell.v) cell.v = ss[parseInt(cell.v)];
		}
		this._cells[r] = row;
	}
	// console.log(this._cells);
}
Sheet.prototype._initMergeCells = function(node) {
	
}
Sheet.prototype._init = function() {
	var child = this._docElem.childNodes;
	for (var i = 0; i < child.length; ++i) {
		if (child[i].nodeName == 'cols') {
			this._initCols(child[i]);
		} else if (child[i].nodeName == 'sheetData') {
			this._initSheetData(child[i]);
		} else if (child[i].nodeName == 'mergeCells') {
			this._initMergeCells(child[i]);
		} else if (child[i].nodeName == 'sheetFormatPr') {
			// this._initSheetFormatPr(child[i]);
		}
	}
}

Workbook.prototype.getSheetNames = function() {
	var ret = [];
	for (var i = 0; i < sheets.length; ++i) {
		ret[i] = sheets[i].name;
	}
	return ret;
}

Workbook.prototype._readSharedString = function(path) {
	this._strings = [];
	var docElem = _readLocalXml(path);
	var child = docElem.childNodes;
	for (var i = 0; i < child.length; ++i) {
		if (child[i].nodeType == 1) {
			var cc = child[i].childNodes;
			for (var j = 0; j < cc.length; ++j) {
				if (cc[j].nodeType == 1) {
					this._strings.push(cc[j].textContent);
					break;
				}
			}
		}
	}
	// console.log(this._strings);
}

Workbook.prototype._readStyle = function() {
}

Workbook.prototype._readSheet = function(sh) {
	if (! this._readySheets) {
		var it = this._zip.findItem(this._partNames._sharedString);
		var p = _partNameToPath(this._destPath, this._partNames._sharedString);
		var b = this._zip.unzipItem(it.index, p);
		this._readSharedString(p);
		
		it = this._zip.findItem(this._partNames._style);
		p = _partNameToPath(this._destPath, this._partNames._style);
		b = this._zip.unzipItem(it.index, p);
		this._readStyle(p);
		this._readySheets = true;
	}
	var p = _partNameToPath(this._destPath, sh.partName);
	var it = this._zip.findItem(sh.partName);
	this._zip.unzipItem(it.index, p);
	
	return _readLocalXml2(p);
}

Workbook.prototype.getSheet = function(idxOrName) {
	var idx = -1;
	if (typeof idxOrName == 'string') {
		for (var i = 0; i < this._sheets.length; ++i) {
			if (this._sheets[i].name == idxOrName) {
				idx = i;
				break;
			}
		}
	} else if (typeof idxOrName == 'number') {
		idx = idxOrName;
	}
	if (idx < 0 || idx >= this._sheets.length) {
		return null;
	}
	var sh = this._sheets[idx];
	if (sh._sheetObj) {
		return sh._sheetObj;
	}
	var rs = this._readSheet(sh);
	sh._sheetObj = new Sheet(this, rs.txt, rs.xml);
	sh._sheetObj._init();
	return sh._sheetObj;
}

var Office = {
	_outPath : '',
	
	_getOutPath : function() {
		if (this._outPath) {
			return this._outPath;
		}
		var buf = createBuffer(512);
		var d = callNative('GetTempPathA', 'i(i,p)', [512, buf.buffer()]);
		var p = buf.toString('GBK');
		p += 'office.js';
		var file = new FILE(p);
		if (! file.isDirectory()) {
			var b = callNative('CreateDirectoryW', 'i(w, p)', [p, 0]);
			if (b) this._outPath = p;
		} else {
			this._outPath = p;
		}
		return this._outPath;
	},
	
	readXLSX : function(path) {
		var f = new ZIP(path);
		if (! f.open()) {
			console.log('XLSX.read() error. File {' + path + '} is not a XLSX file');
			return false;
		}
		return new Workbook(f, this._getOutPath() + '\\' + (new Date().getTime()));
	}
};

