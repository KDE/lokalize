#!/usr/bin/env python
# -*- coding: utf-8 -*-

#python scripts/xliffmerge.py -i tests/testxliffmerge_tr.xlf -t tests/testxliffmerge_en.xlf -o test_merged.xlf

#TODO: mark as 'needs adaptation' when only punctuation is changed
#check id's change after document update

from PyQt4.QtCore import *
from PyQt4.QtXml import *

import itertools
import math

from optparse import OptionParser

parser = OptionParser()
parser.add_option("-I", "--sticky-ids", dest="stickyIds", default=False, action="store_true",
                  help="mark translations as needing review if IDs didn't match")
parser.add_option("-i", "--input", dest="oldFile",
                  help="file with translations")
parser.add_option("-t", "--template", dest="templateFile",
                  help="new template file")
parser.add_option("-o", "--output", dest="outFile",
                  help="where to store merged file")

(options, args) = parser.parse_args()



def saveElement(elem):
    contents=QString()
    stream=QTextStream(contents)
    elem.save(stream,0)
    return contents

def elementText(parent):
    contents=QString()
    n=parent.firstChild()
    while not n.isNull():
        if n.isCharacterData():
            contents+=n.toCharacterData().data()
        elif n.isElement():
            contents+=elementText(n)
        n=n.nextSibling()
    return contents


strings={}
def getDocUnitsList(path):
    doc=QDomDocument()
    file=QFile(path)
    file.open(QIODevice.ReadOnly)
    reader=QXmlSimpleReader()
    reader.setFeature('http://qtsoftware.com/xml/features/report-whitespace-only-CharData',True)
    reader.setFeature('http://xml.org/sax/features/namespaces',False)
    source=QXmlInputSource(file)
    doc.setContent(source,reader)
    file.close()

    units=doc.elementsByTagName("trans-unit")

    unitsList=[]
    for i in range(units.count()):
        unit=units.at(i)
        if unit.firstChildElement("source").text() not in strings:
            strings[unit.firstChildElement("source").text()]=len(strings)
        unitsList.append(strings[unit.firstChildElement("source").text()])

    return (doc, units, unitsList)

(templateDoc, templateUnits, templateUnitsList)=getDocUnitsList(options.templateFile)
(oldDoc, oldUnits, oldUnitsList)=getDocUnitsList(options.oldFile)


freezedOldUnits=[]
for i in range(oldUnits.size()):
    freezedOldUnits.append(oldUnits.at(i))


def lcs_length(xs, ys):
    ny = len(ys)
    curr = list(itertools.repeat(0, 1 + ny))
    for x in xs:
        prev = list(curr)
        for i, y in enumerate(ys):
            if x == y:
                curr[i+1] = prev[i] + 1
            else:
                curr[i+1] = max(curr[i], prev[i+1])
    return curr[ny]


def LCS(X, Y):
    m = len(X)
    n = len(Y)
    # An (m+1) times (n+1) matrix
    C = [[0] * (n+1) for i in range(m+1)]
    for i in range(1, m+1):
        for j in range(1, n+1):
            if X[i-1] == Y[j-1]: 
                C[i][j] = C[i-1][j-1] + 1
            else:
                C[i][j] = max(C[i][j-1], C[i-1][j])
    return C



removedUnits=[]
def recordRemoved(C, X, Y, i, j):
    if i > 0 and j > 0 and X[i-1] == Y[j-1]:
        recordRemoved(C, X, Y, i-1, j-1)
    else:
        C[i-1][j]
        if j > 0 and (i == 0 or C[i][j-1] >= C[i-1][j]):
            recordRemoved(C, X, Y, i, j-1)
        elif i > 0 and (j == 0 or C[i][j-1] < C[i-1][j]):
            recordRemoved(C, X, Y, i-1, j)
            removedUnits.append(i-1)


def inlineTags(parent):
    result=[]
    elem=parent.firstChildElement()
    while not elem.isNull():
        result.append(elem.tagName())
        elem=elem.nextSiblingElement()
    return result

def getIdsMap(parent):
    result={}
    elem=parent.firstChildElement()
    while not elem.isNull():
        result[elem.attribute('id')]=elem
        elem=elem.nextSiblingElement()
    return result

def removeAttributes(elem):
    for attrNode in [elem.attributes().item(i) for i in range(elem.attributes().size())]:
        elem.removeChild(attrNode)

def cloneToAltTrans(unitAltToBe, newUnit):
    altUnit=unitAltToBe.cloneNode().toElement()
    altUnit.setTagName('alt-trans')
    altUnit.setAttribute('alttranstype','previous-version')
    altUnit.setAttribute('phase-name',phaseName)
    altUnit.removeAttribute('id')
    altUnit.removeAttribute('approved')
    refNode=newUnit.firstChildElement('alt-trans')
    if refNode.isNull(): altUnit=newUnit.appendChild(altUnit)
    else: altUnit=newUnit.insertBefore(altUnit,refNode)


    subAltUnits=[]
    subAltUnit=altUnit.firstChildElement('alt-trans')
    refNode=altUnit
    while not subAltUnit.isNull():
        refNode=altUnit.parentNode().insertAfter(subAltUnit.cloneNode(),refNode)
        subAltUnits.append(subAltUnit)
        subAltUnit=subAltUnit.nextSiblingElement('alt-trans')

    for subAltUnit in subAltUnits:
        altUnit.removeChild(subAltUnit)

    return altUnit

INLINE_MARKUP_ELEMENTS=['g', 'x', 'bx', 'ex', 'bpt' , 'ept', 'ph', 'it'] #, 'mrk' -- doesn't have id attribute

globals()['recentlyRemoved']=[]
globals()['lastCommon']=-1
def merge(C, X, Y, i, j):
    if i > 0 and j > 0 and X[i-1] == Y[j-1]:
        merge(C, X, Y, i-1, j-1)
        globals()['recentlyRemoved']=[]
        globals()['lastCommon']=i-1
        templateUnit=templateUnits.at(j-1).toElement()
        templateSource=templateUnit.firstChildElement("source")
        commonUnit=freezedOldUnits[i-1].toElement()
        commonTarget=commonUnit.firstChildElement("target")
        commonSource=commonUnit.firstChildElement("source")
        targetIdsMap=getIdsMap(commonTarget)
        equalIds=False

        # [only] inline markup differs?
        completelyEqual=saveElement(commonSource)==saveElement(templateSource)
        if not completelyEqual:
            altUnit=cloneToAltTrans(commonUnit,commonUnit)
            commonTarget.setAttribute('state','needs-review-adaptation')
            commonUnit.setAttribute('phase-name',phaseName)
            commonTarget.setAttribute('phase-name',phaseName)

        #print ' '+templateSource.text()
        # update inline markup attributes in target
        for markupElement in INLINE_MARKUP_ELEMENTS:

            templateElem=templateSource.firstChildElement(markupElement)
            commonSourceElem=commonSource.firstChildElement(markupElement)
            while not templateElem.isNull() and not commonSourceElem.isNull():
                equalIds=equalIds and commonSourceElem.attribute('id')==templateElem.attribute('id')
                if targetIdsMap.has_key(commonSourceElem.attribute('id')):
                    commonTargetElem=targetIdsMap[commonSourceElem.attribute('id')]
                    removeAttributes(commonTargetElem)
                    for attrNode in [templateElem.attributes().item(i).toAttr() for i in range(templateElem.attributes().size())]:
                        commonTargetElem.setAttribute(attrNode.name(), attrNode.value())

                    del targetIdsMap[commonSourceElem.attribute('id')]

                templateElem=templateElem.nextSiblingElement(markupElement)
                commonSourceElem=commonSourceElem.nextSiblingElement(markupElement)

        # remove inline markup in target which doesn't have corresponding markup in source
        for orphan in targetIdsMap.itervalues():
            if orphan.tagName()=='mrk': continue
            removeAttributes(orphan)
            child=orphan.firstChild()
            while not child.isNull():
                newChild=child.cloneNode()
                orphan.parentNode().insertAfter(newChild,orphan.previousSibling())
                child=child.nextSibling()
            orphan.parentNode().removeChild(orphan)

        #copy templates source entirely
        commonUnit.insertAfter(oldDoc.importNode(templateSource.cloneNode(),True), commonSource)
        commonUnit.removeChild(commonSource)

        #ids
        equalIds=equalIds and commonUnit.attribute('id')==templateUnit.attribute('id')
        if not equalIds and options.stickyIds:
            commonUnit.removeAttribute('approved')

            #if not commonTarget.attribute('state').contains('review'):
            commonTarget.setAttribute('state','needs-review-l10n')
            if not commonTarget.hasChildNodes(): commonTarget.parentNode().removeChild(commonTarget)
        #if equalIds and completelyEqual:
            #commonTarget.setAttribute('state-qualifier','id-match')
        commonUnit.setAttribute('id',templateUnit.attribute('id'))
    else:
        if j > 0 and (i == 0 or C[i][j-1] >= C[i-1][j]):
            merge(C, X, Y, i, j-1)
            templateUnit=templateUnits.at(j-1)
            #print '+'+templateUnit.firstChildElement("source").text()
            newUnit=oldDoc.importNode(templateUnit, True).toElement()
            newUnit.setAttribute('phase-name',phaseName)
            
            if globals()['lastCommon']==-1:
                refNode=freezedOldUnits[0]
                refNode.parentNode().insertBefore(newUnit, refNode)
            else:
                refNode=freezedOldUnits[ globals()['lastCommon'] ]
                refNode.parentNode().insertAfter(newUnit, refNode)
            globals()['lastCommon']=i-1 #to preserve order

            #look for alternate translations, neighbourhood first

            #nonRecentlyRemoved=[x for x in removedUnits if x not in globals()['recentlyRemoved']]
            maxUnits=[]
            newUnitText=newUnit.firstChildElement("source").text()
            newUnitWords=newUnitText.split(' ')

            maxScore=0
            scores={}
            for x in removedUnits:
                remNode=freezedOldUnits[x]
                remNodeText=remNode.firstChildElement("source").text()
                commonWordLen=lcs_length(newUnitWords,remNodeText.split(' '))
                if (commonWordLen+1)<0.5*len(newUnitWords):
                    scores[x]=0
                    continue
                commonLen=lcs_length(newUnitText,remNodeText)
                remLen=newUnitText.size()-commonLen
                addLen=remNodeText.size()-commonLen

                if commonLen==0: score=0
                else: score=99*math.exp(0.2*math.log(1.0*commonLen/newUnitText.size())) / (math.exp(0.015*addLen)*math.exp(0.01*remLen))
                scores[x]=score
                if maxScore<score:maxScore=score

            if maxScore<80: return

            for x in removedUnits:
                if scores[x]==maxScore:
                    remNode=freezedOldUnits[x]
                    maxUnits.append((score+1*(x in globals()['recentlyRemoved']), remNode))

                
            def count_compare_inverted(x, y): return int(y[0]-x[0])
            maxUnits.sort(count_compare_inverted)

            for maxUnit in maxUnits:
                #print maxUnit[0],
                #print newUnitText,
                #print '------------',
                #print maxUnit[1].firstChildElement("source").text()
                cloneToAltTrans(maxUnit[1],newUnit)
        elif i > 0 and (j == 0 or C[i][j-1] < C[i-1][j]):
            merge(C, X, Y, i-1, j)
            globals()['recentlyRemoved'].append(i-1)
            #print '-'+elementText(freezedOldUnits[i-1].toElement().firstChildElement("source"))


def addPhase():
    VERSION='0.1'

    file=oldDoc.elementsByTagName("file").at(0).toElement()
    header=file.firstChildElement("header")
    phasegroup=header.firstChildElement("phase-group")
    if phasegroup.isNull():
        phasegroup=oldDoc.createElement("phase-group")
        #order following XLIFF spec
        skl=header.firstChildElement("skl")
        if not skl.isNull(): header.insertAfter(phasegroup, skl)
        else: header.insertBefore(phasegroup, header.firstChildElement())
    phaseNames={}
    phaseElem=phasegroup.firstChildElement("phase")
    while not phaseElem.isNull():
        phaseNames[phaseElem.attribute("phase-name")]=True
        phaseElem=phaseElem.nextSiblingElement("phase")
    i=1
    while 'update-from-template-%d' % i in phaseNames:
        i+=1

    phaseElem=phasegroup.appendChild(oldDoc.createElement("phase")).toElement()
    phaseElem.setAttribute("phase-name",'update-from-template-%d' % i)

    phaseElem.setAttribute("process-name", 'update-from-template')
    phaseElem.setAttribute("tool-id",      'xliffmerge-%s' % VERSION)
    phaseElem.setAttribute("date",         QDate.currentDate().toString(Qt.ISODate))


    toolElem=header.firstChildElement("tool")
    while not toolElem.isNull() and toolElem.attribute("tool-id")!='xliffmerge-%s' % VERSION:
        toolElem=toolElem.nextSiblingElement("tool")

    if toolElem.isNull():
        toolElem=header.appendChild(oldDoc.createElement("tool")).toElement()
        toolElem.setAttribute("tool-id",'xliffmerge-%s' % VERSION)
        toolElem.setAttribute("tool-name","xliffmerge.py")
        toolElem.setAttribute("tool-version",VERSION)

    return 'update-from-template-%d' % i

phaseName=addPhase()

C = LCS(oldUnitsList, templateUnitsList)

recordRemoved(C, oldUnitsList, templateUnitsList, len(oldUnitsList), len(templateUnitsList))
merge(C, oldUnitsList, templateUnitsList, len(oldUnitsList), len(templateUnitsList))

for remNode in [freezedOldUnits[x] for x in removedUnits]:
    remNode.parentNode().removeChild(remNode)


def fixWhiteSpace(elem):
    first=elem.firstChildElement()
    if not first.previousSibling().isCharacterData():
        elem.insertBefore(oldDoc.createTextNode(''),first)

    n=first
    while not n.isNull():
        if not n.nextSibling().isCharacterData():
            elem.insertAfter(oldDoc.createTextNode(''),n)
        n=n.nextSiblingElement()


def fixWhiteSpaceInList(nodeList):
    for node in [nodeList.at(x) for x in range(nodeList.size())]:
        fixWhiteSpace(node)

containers=["source", "seg-source","target","g","bpt","ept","ph","it","mrk"] #immediate containers allowing markup
for container in containers:
    fixWhiteSpaceInList(oldDoc.elementsByTagName(container))


file=QFile(options.outFile)
file.open(QIODevice.WriteOnly)
stream=QTextStream(file)
oldDoc.save(stream,2)
stream.flush()
file.close()





