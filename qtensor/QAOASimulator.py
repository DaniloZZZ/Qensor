import numpy as np
from tqdm.auto import tqdm
from multiprocessing import Pool
from loguru import logger as log

from qtensor.Simulate import Simulator, QtreeSimulator, CirqSimulator
from qtensor.utils import get_edge_subgraph
from qtensor.lib import graph_hash

class QAOASimulator(Simulator):
    def __init__(self, composer, profile=False, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.composer = composer
        self.profile = profile
        self._subgraph_energy_cache = {}

    def _get_edge_energy(self, G, gamma, beta, edge):
        circuit = self._edge_energy_circuit(G, gamma, beta, edge)
        return self.simulate(circuit)

    def _edge_energy_circuit(self, G, gamma, beta, edge):
        composer = self.composer(G, gamma=gamma, beta=beta)
        composer.energy_expectation_lightcone(edge)

        return composer.circuit


    def _post_process_energy(self, G, E):
        if np.imag(E)>1e-6:
            print(f"Warning: Energy result imaginary part was: {np.imag(E)}")

        """
        C = sum(CC)
        2*CC = 1 - ZZ
        2*C = sum(1-CC)
        2*C = Ed - sum(CC)
        C = (Ed - E)/2
        """
        E = np.real(E)

        Ed = G.number_of_edges()
        return (Ed - E)/2


    def energy_expectation(self, G, gamma, beta):
        """
        Arguments:
            G: MaxCut graph, Networkx
            gamma, beta: list[float]

        Returns: MaxCut energy expectation
        """

        total_E = 0

        with tqdm(total=G.number_of_edges(), desc='Edge iteration', ) as pbar:
            for edge in G.edges():
                E = self._get_edge_energy(G, gamma, beta, edge)
                pbar.set_postfix(Treewidth=self.optimizer.treewidth)
                pbar.update(1)
                total_E += E

            if self.profile:
                print(self.backend.gen_report())

        C = self._post_process_energy(G, total_E)
        return C


    def _parallel_unit_edge(self, args):
        return self._get_edge_energy(*args)

    def energy_expectation_parallel(self, G, gamma, beta, n_processes=4):
        """
        Arguments:
            G: MaxCut graph, Networkx
            gamma, beta: list[float]

        Returns: MaxCut energy expectation
        """
        args = [(G, gamma, beta, edge) for edge in G.edges()]

        with Pool(n_processes) as p:

           r = list(tqdm(p.imap(self._parallel_unit_edge, args), total=G.number_of_edges()))
           total_E = sum(r)
        C = self._post_process_energy(G, total_E)

        return C

class CachedQAOASimulator(QAOASimulator):
    def __init__(self, composer, profile=False, *args, **kwargs):
        super().__init__(composer, profile=profile, *args, **kwargs)
        self._subgraph_energy_cache = {}

    def _get_edge_energy(self, G, gamma, beta, edge):
        graph = get_edge_subgraph(G, edge, len(gamma))
        ghash = graph_hash(graph)
        cached_value = self._subgraph_energy_cache.get(ghash)
        if cached_value is None:
            E = super()._get_edge_energy(G, gamma, beta, edge)
            self._subgraph_energy_cache[ghash] = E
            return E
        else:
            return cached_value

class QAOAQtreeSimulator(QAOASimulator, QtreeSimulator):
    pass

class QAOACirqSimulator(QAOASimulator, CirqSimulator):
    def _get_edge_energy(self, G, gamma, beta, edge):
        self.max_tw = 25
        if not hasattr(self, '_warned'):
            print('Warning: the energy calculation is not yet implemented')
            self._warned = True
        circuit = self._edge_energy_circuit(G, gamma, beta, edge)
        trial_result = self.simulate(circuit)
        return np.sum(trial_result.state_vector())
    pass
