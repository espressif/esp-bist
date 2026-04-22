"""BM25-based search engine for ESP-BIST MCP server.

Provides lightweight text search over pre-built JSON data files
with metadata filtering by content_type and soc_target.
"""

import json
import re
from pathlib import Path
from typing import Optional

try:
    from rank_bm25 import BM25Okapi
except ImportError:
    BM25Okapi = None


def tokenize(text: str) -> list[str]:
    """Simple whitespace + punctuation tokenizer with lowercasing."""
    text = text.lower()
    text = re.sub(r"[^a-z0-9_]+", " ", text)
    return [t for t in text.split() if len(t) > 1]


class SearchIndex:
    """BM25 search index over a list of documents."""

    def __init__(self, documents: list[dict], text_fields: list[str]):
        self.documents = documents
        self.text_fields = text_fields
        self._corpus = []

        for doc in documents:
            combined = " ".join(str(doc.get(f, "")) for f in text_fields)
            self._corpus.append(tokenize(combined))

        if BM25Okapi and self._corpus:
            self._bm25 = BM25Okapi(self._corpus)
        else:
            self._bm25 = None

    def search(
        self,
        query: str,
        top_k: int = 10,
        filters: Optional[dict] = None,
    ) -> list[dict]:
        """Search documents, optionally filtering by metadata fields."""
        if not self.documents:
            return []

        query_tokens = tokenize(query)
        if not query_tokens:
            return []

        if self._bm25 is not None:
            scores = self._bm25.get_scores(query_tokens)
        else:
            scores = self._fallback_scores(query_tokens)

        scored_docs = list(zip(scores, range(len(self.documents)), self.documents))

        if filters:
            filtered = []
            for score, idx, doc in scored_docs:
                match = True
                for key, value in filters.items():
                    if value is None:
                        continue
                    doc_val = doc.get(key, "")
                    if isinstance(doc_val, str):
                        if value.lower() not in doc_val.lower():
                            match = False
                            break
                    elif isinstance(doc_val, list):
                        if not any(value.lower() in str(v).lower() for v in doc_val):
                            match = False
                            break
                if match:
                    filtered.append((score, idx, doc))
            scored_docs = filtered

        scored_docs.sort(key=lambda x: x[0], reverse=True)

        results = []
        for score, idx, doc in scored_docs[:top_k]:
            if score <= 0:
                continue
            result = {**doc, "_score": round(float(score), 4)}
            results.append(result)

        return results

    def _fallback_scores(self, query_tokens: list[str]) -> list[float]:
        """Simple term-frequency fallback when BM25 is not available."""
        scores = []
        for corpus_tokens in self._corpus:
            token_set = set(corpus_tokens)
            score = sum(1.0 for qt in query_tokens if qt in token_set)
            scores.append(score)
        return scores


class BISTSearchEngine:
    """Composite search engine loading all ESP-BIST data indexes."""

    def __init__(self, data_dir: str | Path):
        self.data_dir = Path(data_dir)
        self._indexes: dict[str, SearchIndex] = {}
        self._raw: dict[str, list] = {}
        self._load()

    def _load(self):
        """Load JSON data files and build search indexes."""
        configs = {
            "docs": ["title", "section", "body"],
            "api": ["function", "signature", "module", "description", "returns", "note"],
            "kconfig": ["name", "prompt", "help", "menu", "depends_on"],
            "source": ["function_name", "body", "module"],
            "socs": ["soc", "name", "cpu", "specific_notes"],
        }

        for name, fields in configs.items():
            filepath = self.data_dir / f"{name}.json"
            if filepath.exists():
                with open(filepath) as f:
                    data = json.load(f)
                self._raw[name] = data
                self._indexes[name] = SearchIndex(data, fields)
                print(f"  Loaded {name}: {len(data)} entries")
            else:
                self._raw[name] = []
                self._indexes[name] = SearchIndex([], fields)
                print(f"  Warning: {filepath} not found")

    def search_docs(
        self, query: str, top_k: int = 10, soc_target: Optional[str] = None
    ) -> list[dict]:
        """Search documentation chunks.

        When *soc_target* is given the search first tries to filter by it;
        if that yields no results it falls back to an unfiltered search so
        the caller still gets useful data.
        """
        if soc_target:
            results = self._indexes["docs"].search(
                query, top_k=top_k, filters={"body": soc_target}
            )
            if results:
                return results
        return self._indexes["docs"].search(query, top_k=top_k)

    def search_api(
        self, query: str, top_k: int = 10, soc_target: Optional[str] = None
    ) -> list[dict]:
        """Search API reference entries.

        When *soc_target* is given the search first tries to filter by it;
        if that yields no results it falls back to an unfiltered search so
        the caller still gets useful data.
        """
        if soc_target:
            results = self._indexes["api"].search(
                query, top_k=top_k, filters={"description": soc_target}
            )
            if results:
                return results
        return self._indexes["api"].search(query, top_k=top_k)

    def get_api_by_name(self, name: str) -> list[dict]:
        """Look up API entry by exact or partial function/type name."""
        name_lower = name.lower()
        results = []
        for entry in self._raw.get("api", []):
            func = entry.get("function", "").lower()
            if name_lower == func or name_lower in func:
                results.append(entry)
        return results

    def search_kconfig(self, query: str, top_k: int = 10) -> list[dict]:
        """Search Kconfig options."""
        query_upper = query.upper().replace("CONFIG_", "")
        exact = [
            e
            for e in self._raw.get("kconfig", [])
            if query_upper in e.get("name", "").upper()
        ]
        if exact:
            return exact[:top_k]
        return self._indexes["kconfig"].search(query, top_k=top_k)

    def search_source(
        self,
        query: str,
        top_k: int = 10,
        file_type: Optional[str] = None,
    ) -> list[dict]:
        """Search source code chunks."""
        results = self._indexes["source"].search(query, top_k=top_k * 3)

        if file_type and file_type != "all":
            ext = ".h" if file_type == "header" else ".c"
            results = [r for r in results if str(r.get("file", "")).endswith(ext)]

        return results[:top_k]

    def get_socs(self, soc_target: Optional[str] = None) -> list[dict]:
        """Get SoC information."""
        socs = self._raw.get("socs", [])
        if soc_target:
            target = soc_target.lower().replace("-", "")
            return [s for s in socs if target in s.get("soc", "").lower().replace("-", "")]
        return socs

    def search_architecture(
        self, topic: str, top_k: int = 10, soc_target: Optional[str] = None
    ) -> list[dict]:
        """Search architecture-related documentation."""
        arch_keywords = [
            "architecture",
            "design",
            "module",
            "layer",
            "memory",
            "interrupt",
            "build",
            "system",
            "flow",
            "structure",
        ]
        boosted_query = f"{topic} {' '.join(arch_keywords)}"

        arch_docs = [
            d
            for d in self._raw.get("docs", [])
            if any(
                kw in d.get("source_file", "").lower()
                for kw in [
                    "architecture",
                    "module_design",
                    "application_guide",
                    "get_started",
                    "safety",
                ]
            )
        ]

        if arch_docs:
            idx = SearchIndex(arch_docs, ["title", "section", "body"])
            results = idx.search(topic, top_k=top_k)
            if results:
                return results

        return self.search_docs(boosted_query, top_k=top_k, soc_target=soc_target)
