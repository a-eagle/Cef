function _readLocalXml(path) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open("GET", path, false); // false: sync request
	xmlhttp.send(null);
	var docElem = xmlhttp.responseXML.documentElement;
	return docElem;
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
	console.log(sheets);
}

function Sheet(txt, docElem) {
	this._txt = txt;
	this._docElem = docElem;
}

Sheet.prototype._init = function() {
	
}

Workbook.prototype.getSheetNames = function() {
	var ret = [];
	for (var i = 0; i < sheets.length; ++i) {
		ret[i] = sheets[i].name;
	}
	return ret;
}

Workbook.prototype._readSheet = function(sh) {
	if (! this._readySheets) {
		var it = this._zip.findItem(this._partNames._sharedString);
		var b = this._zip.unzipItem(it.index, _partNameToPath(this._destPath, this._partNames._sharedString));
		it = this._zip.findItem(this._partNames._style);
		b = this._zip.unzipItem(it.index, _partNameToPath(this._destPath, this._partNames._style));
		this._readySheets = true;
	}
	var p = _partNameToPath(this._destPath, sh.partName);
	var it = this._zip.findItem(sh.partName);
	this._zip.unzipItem(it.index, p);
	
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open("GET", p, false); // false: sync request
	xmlhttp.send(null);
	// console.log(xmlhttp.responseText);
	return {txt: xmlhttp.responseText, xml: xmlhttp.responseXML};
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
	sh._sheetObj = new Sheet(rs.txt, rs.xml);
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


