package hycallparser.builtinProcedures;

import org.reflections.Reflections;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class ProcedureManager {

    private static List<Procedure> procs;

    static {
        procs = new ArrayList<>();
        for (Class c : new Reflections("hycallparser.builtinProcedures").getSubTypesOf(hycallparser.builtinProcedures.Procedure.class)) {
            try {
                Procedure p = (Procedure)c.getConstructor().newInstance();
                if (getProcedure(p.getName(), p.getParameterCount()).isPresent())
                    throw new IllegalArgumentException("Two builtin procedures called \"" + p.getName() + "\" with " + p.getParameterCount() + " exist!");
                procs.add(p);
            } catch (Exception e) {
                throw new IllegalArgumentException("Exception occured during instantiation of Procedure " + c.getName() + "!");
            }
        }
    }

    public static boolean existsBuiltinWithName(String name) {
        return procs.stream().anyMatch(p -> p.getName().equals(name));
    }

    public static Optional<Procedure> getProcedure(String name, int parameterCount) {
        return procs.stream().filter(p -> p.getName().equals(name)).filter(p -> p.getParameterCount() == parameterCount).findFirst();
    }
}
